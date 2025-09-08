#include "file_storage.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace snipper::persistence {

FileStorage::FileStorage(const std::string& filePath, bool autoSave) 
    : filePath_(filePath), connected_(false), inTransaction_(false), autoSave_(autoSave) {
}

FileStorage::~FileStorage() {
    if (connected_) {
        disconnect();
    }
}

bool FileStorage::insert(const DataRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    records_[record.id] = record;
    
    if (autoSave_) {
        return saveToFile();
    }
    
    return true;
}

bool FileStorage::update(const std::string& id, const json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.find(id);
    if (it != records_.end()) {
        it->second.data = data;
        
        if (autoSave_) {
            return saveToFile();
        }
        return true;
    }
    
    return false;
}

bool FileStorage::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.find(id);
    if (it != records_.end()) {
        records_.erase(it);
        
        if (autoSave_) {
            return saveToFile();
        }
        return true;
    }
    
    return false;
}

DataRecord FileStorage::findById(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return DataRecord();
    }
    
    auto it = records_.find(id);
    if (it != records_.end()) {
        return it->second;
    }
    
    return DataRecord();
}

QueryResult FileStorage::query(const std::vector<QueryCondition>& conditions, 
                              size_t offset, size_t limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    QueryResult result;
    result.offset = offset;
    result.limit = limit;
    
    if (!connected_) {
        return result;
    }
    
    std::vector<DataRecord> matchingRecords;
    
    for (const auto& pair : records_) {
        if (matchesConditions(pair.second, conditions)) {
            matchingRecords.push_back(pair.second);
        }
    }
    
    // 按时间戳排序（最新的在前）
    std::sort(matchingRecords.begin(), matchingRecords.end(),
              [](const DataRecord& a, const DataRecord& b) {
                  return a.timestamp > b.timestamp;
              });
    
    result.totalCount = matchingRecords.size();
    
    // 分页
    size_t start = std::min(offset, matchingRecords.size());
    size_t end = std::min(start + limit, matchingRecords.size());
    
    for (size_t i = start; i < end; ++i) {
        result.records.push_back(matchingRecords[i]);
    }
    
    return result;
}

QueryResult FileStorage::queryByType(const std::string& type, 
                                    size_t offset, size_t limit) {
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", type);
    return query(conditions, offset, limit);
}

QueryResult FileStorage::queryByTimeRange(const std::chrono::system_clock::time_point& start,
                                         const std::chrono::system_clock::time_point& end,
                                         size_t offset, size_t limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    QueryResult result;
    result.offset = offset;
    result.limit = limit;
    
    if (!connected_) {
        return result;
    }
    
    std::vector<DataRecord> matchingRecords;
    
    for (const auto& pair : records_) {
        const auto& record = pair.second;
        if (record.timestamp >= start && record.timestamp <= end) {
            matchingRecords.push_back(record);
        }
    }
    
    // 按时间戳排序（最新的在前）
    std::sort(matchingRecords.begin(), matchingRecords.end(),
              [](const DataRecord& a, const DataRecord& b) {
                  return a.timestamp > b.timestamp;
              });
    
    result.totalCount = matchingRecords.size();
    
    // 分页
    size_t startIdx = std::min(offset, matchingRecords.size());
    size_t endIdx = std::min(startIdx + limit, matchingRecords.size());
    
    for (size_t i = startIdx; i < endIdx; ++i) {
        result.records.push_back(matchingRecords[i]);
    }
    
    return result;
}

size_t FileStorage::count(const std::vector<QueryCondition>& conditions) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return 0;
    }
    
    size_t count = 0;
    for (const auto& pair : records_) {
        if (matchesConditions(pair.second, conditions)) {
            count++;
        }
    }
    
    return count;
}

size_t FileStorage::countByType(const std::string& type) {
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", type);
    return count(conditions);
}

bool FileStorage::cleanup(const std::chrono::system_clock::time_point& before) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.begin();
    while (it != records_.end()) {
        if (it->second.timestamp < before) {
            it = records_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (autoSave_) {
        return saveToFile();
    }
    
    return true;
}

bool FileStorage::cleanupByType(const std::string& type, 
                               const std::chrono::system_clock::time_point& before) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.begin();
    while (it != records_.end()) {
        if (it->second.type == type && it->second.timestamp < before) {
            it = records_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (autoSave_) {
        return saveToFile();
    }
    
    return true;
}

bool FileStorage::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 确保目录存在
    std::filesystem::path path(filePath_);
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    
    // 加载现有数据
    if (std::filesystem::exists(filePath_)) {
        if (!loadFromFile()) {
            return false;
        }
    }
    
    connected_ = true;
    return true;
}

bool FileStorage::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connected_) {
        if (autoSave_) {
            saveToFile();
        }
        connected_ = false;
    }
    
    return true;
}

bool FileStorage::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

bool FileStorage::beginTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || inTransaction_) {
        return false;
    }
    inTransaction_ = true;
    return true;
}

bool FileStorage::commitTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || !inTransaction_) {
        return false;
    }
    
    bool result = true;
    if (autoSave_) {
        result = saveToFile();
    }
    
    inTransaction_ = false;
    return result;
}

bool FileStorage::rollbackTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || !inTransaction_) {
        return false;
    }
    
    // 重新加载文件以回滚更改
    if (std::filesystem::exists(filePath_)) {
        loadFromFile();
    } else {
        records_.clear();
    }
    
    inTransaction_ = false;
    return true;
}

bool FileStorage::setAutoSave(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    autoSave_ = enabled;
    return true;
}

bool FileStorage::forceSave() {
    std::lock_guard<std::mutex> lock(mutex_);
    return saveToFile();
}

bool FileStorage::backup(const std::string& backupPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::filesystem::copy_file(filePath_, backupPath, 
                                  std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool FileStorage::restore(const std::string& backupPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!std::filesystem::exists(backupPath)) {
        return false;
    }
    
    try {
        std::filesystem::copy_file(backupPath, filePath_, 
                                  std::filesystem::copy_options::overwrite_existing);
        return loadFromFile();
    } catch (const std::exception& e) {
        return false;
    }
}

bool FileStorage::loadFromFile() {
    try {
        std::ifstream file(filePath_);
        if (!file.is_open()) {
            return false;
        }
        
        json data;
        file >> data;
        
        records_.clear();
        
        if (data.is_array()) {
            for (const auto& item : data) {
                DataRecord record;
                record.id = item.value("id", "");
                record.type = item.value("type", "");
                record.data = item.value("data", json::object());
                record.source = item.value("source", "");
                
                // 解析时间戳
                if (item.contains("timestamp")) {
                    auto timestampMs = item["timestamp"].get<int64_t>();
                    record.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(timestampMs));
                } else {
                    record.timestamp = std::chrono::system_clock::now();
                }
                
                if (!record.id.empty()) {
                    records_[record.id] = record;
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool FileStorage::saveToFile() {
    try {
        std::ofstream file(filePath_);
        if (!file.is_open()) {
            return false;
        }
        
        json data = json::array();
        
        for (const auto& pair : records_) {
            json record;
            record["id"] = pair.second.id;
            record["type"] = pair.second.type;
            record["data"] = pair.second.data;
            record["source"] = pair.second.source;
            
            // 保存时间戳为毫秒
            auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                pair.second.timestamp.time_since_epoch()).count();
            record["timestamp"] = timestampMs;
            
            data.push_back(record);
        }
        
        file << data.dump(2);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string FileStorage::generateId() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

bool FileStorage::matchesConditions(const DataRecord& record, 
                                  const std::vector<QueryCondition>& conditions) const {
    if (conditions.empty()) {
        return true;
    }
    
    bool result = true;
    std::string logic = "AND";
    
    for (const auto& condition : conditions) {
        bool conditionResult = matchesCondition(record, condition);
        
        if (logic == "AND") {
            result = result && conditionResult;
        } else if (logic == "OR") {
            result = result || conditionResult;
        }
        
        logic = condition.logic;
    }
    
    return result;
}

bool FileStorage::matchesCondition(const DataRecord& record, 
                                 const QueryCondition& condition) const {
    // 特殊字段处理
    if (condition.field == "id") {
        return compareValues(record.id, condition.operator_, condition.value);
    } else if (condition.field == "type") {
        return compareValues(record.type, condition.operator_, condition.value);
    } else if (condition.field == "source") {
        return compareValues(record.source, condition.operator_, condition.value);
    } else if (condition.field == "timestamp") {
        // 时间戳比较需要特殊处理
        auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.timestamp.time_since_epoch()).count();
        return compareValues(timestampMs, condition.operator_, condition.value);
    } else {
        // 在data字段中查找
        if (record.data.contains(condition.field)) {
            return compareValues(record.data[condition.field], condition.operator_, condition.value);
        }
    }
    
    return false;
}

bool FileStorage::compareValues(const json& recordValue, const std::string& op, 
                              const json& conditionValue) const {
    try {
        if (op == "==") {
            return recordValue == conditionValue;
        } else if (op == "!=") {
            return recordValue != conditionValue;
        } else if (op == ">") {
            return recordValue > conditionValue;
        } else if (op == ">=") {
            return recordValue >= conditionValue;
        } else if (op == "<") {
            return recordValue < conditionValue;
        } else if (op == "<=") {
            return recordValue <= conditionValue;
        } else if (op == "contains") {
            if (recordValue.is_string() && conditionValue.is_string()) {
                return recordValue.get<std::string>().find(conditionValue.get<std::string>()) != std::string::npos;
            }
        } else if (op == "starts_with") {
            if (recordValue.is_string() && conditionValue.is_string()) {
                const std::string& recordStr = recordValue.get<std::string>();
                const std::string& conditionStr = conditionValue.get<std::string>();
                return recordStr.length() >= conditionStr.length() && 
                       recordStr.substr(0, conditionStr.length()) == conditionStr;
            }
        } else if (op == "ends_with") {
            if (recordValue.is_string() && conditionValue.is_string()) {
                const std::string& recordStr = recordValue.get<std::string>();
                const std::string& conditionStr = conditionValue.get<std::string>();
                return recordStr.length() >= conditionStr.length() && 
                       recordStr.substr(recordStr.length() - conditionStr.length()) == conditionStr;
            }
        }
    } catch (const std::exception& e) {
        // 比较失败，返回false
    }
    
    return false;
}

} // namespace snipper::persistence
