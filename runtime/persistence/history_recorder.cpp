#include "history_recorder.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace snipper::persistence {

HistoryRecorder::HistoryRecorder(std::unique_ptr<StorageInterface> storage, 
                               const std::string& source)
    : storage_(std::move(storage)), source_(source) {
}

bool HistoryRecorder::recordRuleExecution(const RuleExecutionRecord& record) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    DataRecord dataRecord = createRuleExecutionDataRecord(record);
    return storage_->insert(dataRecord);
}

bool HistoryRecorder::recordRuleStart(const std::string& ruleId, const std::string& ruleName, 
                                     const json& context) {
    RuleExecutionRecord record(ruleId, ruleName);
    record.context = context;
    record.executed = true;
    record.startTime = std::chrono::system_clock::now();
    
    return recordRuleExecution(record);
}

bool HistoryRecorder::recordRuleEnd(const std::string& ruleId, bool success, 
                                   const std::string& errorMessage, const json& actions) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    // 查找对应的开始记录
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("rule_id", "==", ruleId);
    conditions.emplace_back("executed", "==", true);
    conditions.emplace_back("success", "==", false); // 未完成的记录
    
    QueryResult result = storage_->query(conditions, 0, 1);
    if (result.records.empty()) {
        return false;
    }
    
    // 更新记录
    auto endTime = std::chrono::system_clock::now();
    auto startTime = result.records[0].timestamp;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    json updateData = result.records[0].data;
    updateData["success"] = success;
    updateData["error_message"] = errorMessage;
    updateData["actions"] = actions;
    updateData["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime.time_since_epoch()).count();
    updateData["duration_ms"] = duration.count();
    
    return storage_->update(result.records[0].id, updateData);
}

bool HistoryRecorder::recordSensorData(const SensorDataRecord& record) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    DataRecord dataRecord = createSensorDataRecord(record);
    return storage_->insert(dataRecord);
}

bool HistoryRecorder::recordSensorData(const std::string& sensorId, const std::string& sensorType,
                                      const json& value, const std::string& unit,
                                      const std::string& location) {
    SensorDataRecord record(sensorId, sensorType, value, unit, location);
    return recordSensorData(record);
}

QueryResult HistoryRecorder::getRuleExecutionHistory(const std::string& ruleId, 
                                                    size_t offset, size_t limit) {
    if (!storage_ || !storage_->isConnected()) {
        return QueryResult();
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_execution");
    
    if (!ruleId.empty()) {
        conditions.emplace_back("rule_id", "==", ruleId);
    }
    
    return storage_->query(conditions, offset, limit);
}

QueryResult HistoryRecorder::getRuleExecutionHistory(const std::chrono::system_clock::time_point& start,
                                                    const std::chrono::system_clock::time_point& end,
                                                    size_t offset, size_t limit) {
    if (!storage_ || !storage_->isConnected()) {
        return QueryResult();
    }
    
    // 先按类型过滤
    QueryResult result = storage_->queryByType("rule_execution", 0, 10000); // 获取所有记录
    
    // 手动过滤时间范围
    QueryResult filteredResult;
    filteredResult.offset = offset;
    filteredResult.limit = limit;
    
    std::vector<DataRecord> timeFiltered;
    for (const auto& record : result.records) {
        if (record.timestamp >= start && record.timestamp <= end) {
            timeFiltered.push_back(record);
        }
    }
    
    // 按时间排序
    std::sort(timeFiltered.begin(), timeFiltered.end(),
              [](const DataRecord& a, const DataRecord& b) {
                  return a.timestamp > b.timestamp;
              });
    
    filteredResult.totalCount = timeFiltered.size();
    
    // 分页
    size_t startIdx = std::min(offset, timeFiltered.size());
    size_t endIdx = std::min(startIdx + limit, timeFiltered.size());
    
    for (size_t i = startIdx; i < endIdx; ++i) {
        filteredResult.records.push_back(timeFiltered[i]);
    }
    
    return filteredResult;
}

QueryResult HistoryRecorder::getSensorDataHistory(const std::string& sensorId,
                                                 size_t offset, size_t limit) {
    if (!storage_ || !storage_->isConnected()) {
        return QueryResult();
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "sensor_data");
    
    if (!sensorId.empty()) {
        conditions.emplace_back("sensor_id", "==", sensorId);
    }
    
    return storage_->query(conditions, offset, limit);
}

QueryResult HistoryRecorder::getSensorDataHistory(const std::string& sensorId,
                                                 const std::chrono::system_clock::time_point& start,
                                                 const std::chrono::system_clock::time_point& end,
                                                 size_t offset, size_t limit) {
    if (!storage_ || !storage_->isConnected()) {
        return QueryResult();
    }
    
    // 先按类型和传感器ID过滤
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "sensor_data");
    if (!sensorId.empty()) {
        conditions.emplace_back("sensor_id", "==", sensorId);
    }
    
    QueryResult result = storage_->query(conditions, 0, 10000);
    
    // 手动过滤时间范围
    QueryResult filteredResult;
    filteredResult.offset = offset;
    filteredResult.limit = limit;
    
    std::vector<DataRecord> timeFiltered;
    for (const auto& record : result.records) {
        if (record.timestamp >= start && record.timestamp <= end) {
            timeFiltered.push_back(record);
        }
    }
    
    // 按时间排序
    std::sort(timeFiltered.begin(), timeFiltered.end(),
              [](const DataRecord& a, const DataRecord& b) {
                  return a.timestamp > b.timestamp;
              });
    
    filteredResult.totalCount = timeFiltered.size();
    
    // 分页
    size_t startIdx = std::min(offset, timeFiltered.size());
    size_t endIdx = std::min(startIdx + limit, timeFiltered.size());
    
    for (size_t i = startIdx; i < endIdx; ++i) {
        filteredResult.records.push_back(timeFiltered[i]);
    }
    
    return filteredResult;
}

size_t HistoryRecorder::getRuleExecutionCount(const std::string& ruleId) {
    if (!storage_ || !storage_->isConnected()) {
        return 0;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_execution");
    
    if (!ruleId.empty()) {
        conditions.emplace_back("rule_id", "==", ruleId);
    }
    
    return storage_->count(conditions);
}

size_t HistoryRecorder::getSensorDataCount(const std::string& sensorId) {
    if (!storage_ || !storage_->isConnected()) {
        return 0;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "sensor_data");
    
    if (!sensorId.empty()) {
        conditions.emplace_back("sensor_id", "==", sensorId);
    }
    
    return storage_->count(conditions);
}

double HistoryRecorder::getRuleSuccessRate(const std::string& ruleId) {
    if (!storage_ || !storage_->isConnected()) {
        return 0.0;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_execution");
    
    if (!ruleId.empty()) {
        conditions.emplace_back("rule_id", "==", ruleId);
    }
    
    size_t totalCount = storage_->count(conditions);
    if (totalCount == 0) {
        return 0.0;
    }
    
    // 计算成功次数
    conditions.emplace_back("success", "==", true);
    size_t successCount = storage_->count(conditions);
    
    return static_cast<double>(successCount) / totalCount;
}

double HistoryRecorder::getAverageRuleExecutionTime(const std::string& ruleId) {
    if (!storage_ || !storage_->isConnected()) {
        return 0.0;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_execution");
    
    if (!ruleId.empty()) {
        conditions.emplace_back("rule_id", "==", ruleId);
    }
    
    QueryResult result = storage_->query(conditions, 0, 10000);
    
    if (result.records.empty()) {
        return 0.0;
    }
    
    int64_t totalDuration = 0;
    size_t validRecords = 0;
    
    for (const auto& record : result.records) {
        if (record.data.contains("duration_ms")) {
            totalDuration += record.data["duration_ms"].get<int64_t>();
            validRecords++;
        }
    }
    
    if (validRecords == 0) {
        return 0.0;
    }
    
    return static_cast<double>(totalDuration) / validRecords;
}

bool HistoryRecorder::cleanupRuleHistory(const std::chrono::system_clock::time_point& before) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    return storage_->cleanupByType("rule_execution", before);
}

bool HistoryRecorder::cleanupSensorData(const std::chrono::system_clock::time_point& before) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    return storage_->cleanupByType("sensor_data", before);
}

bool HistoryRecorder::cleanupByRuleId(const std::string& ruleId) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_execution");
    conditions.emplace_back("rule_id", "==", ruleId);
    
    QueryResult result = storage_->query(conditions, 0, 10000);
    
    for (const auto& record : result.records) {
        storage_->remove(record.id);
    }
    
    return true;
}

bool HistoryRecorder::cleanupBySensorId(const std::string& sensorId) {
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "sensor_data");
    conditions.emplace_back("sensor_id", "==", sensorId);
    
    QueryResult result = storage_->query(conditions, 0, 10000);
    
    for (const auto& record : result.records) {
        storage_->remove(record.id);
    }
    
    return true;
}

bool HistoryRecorder::connect() {
    if (!storage_) {
        return false;
    }
    return storage_->connect();
}

bool HistoryRecorder::disconnect() {
    if (!storage_) {
        return false;
    }
    return storage_->disconnect();
}

bool HistoryRecorder::isConnected() const {
    return storage_ && storage_->isConnected();
}

std::string HistoryRecorder::generateId() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 16; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

DataRecord HistoryRecorder::createRuleExecutionDataRecord(const RuleExecutionRecord& record) const {
    DataRecord dataRecord;
    dataRecord.id = generateId();
    dataRecord.type = "rule_execution";
    dataRecord.source = source_;
    dataRecord.timestamp = record.startTime;
    
    json data;
    data["rule_id"] = record.ruleId;
    data["rule_name"] = record.ruleName;
    data["executed"] = record.executed;
    data["success"] = record.success;
    data["error_message"] = record.errorMessage;
    data["context"] = record.context;
    data["actions"] = record.actions;
    data["start_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        record.startTime.time_since_epoch()).count();
    data["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        record.endTime.time_since_epoch()).count();
    data["duration_ms"] = record.durationMs;
    
    dataRecord.data = data;
    return dataRecord;
}

DataRecord HistoryRecorder::createSensorDataRecord(const SensorDataRecord& record) const {
    DataRecord dataRecord;
    dataRecord.id = generateId();
    dataRecord.type = "sensor_data";
    dataRecord.source = source_;
    dataRecord.timestamp = record.timestamp;
    
    json data;
    data["sensor_id"] = record.sensorId;
    data["sensor_type"] = record.sensorType;
    data["value"] = record.value;
    data["unit"] = record.unit;
    data["location"] = record.location;
    
    dataRecord.data = data;
    return dataRecord;
}

} // namespace snipper::persistence
