#include "memory_storage.h"
#include <sstream>
#include <iomanip>

namespace snipper::persistence {

MemoryStorage::MemoryStorage() : connected_(false), inTransaction_(false) {
}

bool MemoryStorage::insert(const DataRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    records_[record.id] = record;
    return true;
}

bool MemoryStorage::update(const std::string& id, const json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.find(id);
    if (it != records_.end()) {
        it->second.data = data;
        return true;
    }
    
    return false;
}

bool MemoryStorage::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!connected_) {
        return false;
    }
    
    auto it = records_.find(id);
    if (it != records_.end()) {
        records_.erase(it);
        return true;
    }
    
    return false;
}

DataRecord MemoryStorage::findById(const std::string& id) {
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

QueryResult MemoryStorage::query(const std::vector<QueryCondition>& conditions, 
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

QueryResult MemoryStorage::queryByType(const std::string& type, 
                                      size_t offset, size_t limit) {
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", type);
    return query(conditions, offset, limit);
}

QueryResult MemoryStorage::queryByTimeRange(const std::chrono::system_clock::time_point& start,
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

size_t MemoryStorage::count(const std::vector<QueryCondition>& conditions) {
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

size_t MemoryStorage::countByType(const std::string& type) {
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", type);
    return count(conditions);
}

bool MemoryStorage::cleanup(const std::chrono::system_clock::time_point& before) {
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
    
    return true;
}

bool MemoryStorage::cleanupByType(const std::string& type, 
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
    
    return true;
}

bool MemoryStorage::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = true;
    return true;
}

bool MemoryStorage::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
    return true;
}

bool MemoryStorage::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

bool MemoryStorage::beginTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || inTransaction_) {
        return false;
    }
    inTransaction_ = true;
    return true;
}

bool MemoryStorage::commitTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || !inTransaction_) {
        return false;
    }
    inTransaction_ = false;
    return true;
}

bool MemoryStorage::rollbackTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || !inTransaction_) {
        return false;
    }
    inTransaction_ = false;
    return true;
}

size_t MemoryStorage::getTotalRecords() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_.size();
}

void MemoryStorage::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

std::vector<std::string> MemoryStorage::getAllIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(records_.size());
    
    for (const auto& pair : records_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

bool MemoryStorage::matchesConditions(const DataRecord& record, 
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

bool MemoryStorage::matchesCondition(const DataRecord& record, 
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

bool MemoryStorage::compareValues(const json& recordValue, const std::string& op, 
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
