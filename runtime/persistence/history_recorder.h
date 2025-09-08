#pragma once

#include "storage_interface.h"
#include <memory>
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace snipper::persistence {

using json = nlohmann::json;

// 规则执行记录
struct RuleExecutionRecord {
    std::string ruleId;
    std::string ruleName;
    bool executed;
    bool success;
    std::string errorMessage;
    json context;
    json actions;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    int64_t durationMs;
    
    RuleExecutionRecord() = default;
    RuleExecutionRecord(const std::string& ruleId, const std::string& ruleName)
        : ruleId(ruleId), ruleName(ruleName), executed(false), success(false), 
          startTime(std::chrono::system_clock::now()) {}
};

// 传感器数据记录
struct SensorDataRecord {
    std::string sensorId;
    std::string sensorType;
    json value;
    std::string unit;
    std::string location;
    std::chrono::system_clock::time_point timestamp;
    
    SensorDataRecord() = default;
    SensorDataRecord(const std::string& sensorId, const std::string& sensorType, 
                    const json& value, const std::string& unit = "", 
                    const std::string& location = "")
        : sensorId(sensorId), sensorType(sensorType), value(value), unit(unit), 
          location(location), timestamp(std::chrono::system_clock::now()) {}
};

// 历史记录器
class HistoryRecorder {
private:
    std::unique_ptr<StorageInterface> storage_;
    std::string source_;
    
public:
    explicit HistoryRecorder(std::unique_ptr<StorageInterface> storage, 
                           const std::string& source = "snipper");
    ~HistoryRecorder() = default;
    
    // 规则执行记录
    bool recordRuleExecution(const RuleExecutionRecord& record);
    bool recordRuleStart(const std::string& ruleId, const std::string& ruleName, 
                        const json& context = json::object());
    bool recordRuleEnd(const std::string& ruleId, bool success, 
                      const std::string& errorMessage = "", 
                      const json& actions = json::array());
    
    // 传感器数据记录
    bool recordSensorData(const SensorDataRecord& record);
    bool recordSensorData(const std::string& sensorId, const std::string& sensorType,
                         const json& value, const std::string& unit = "",
                         const std::string& location = "");
    
    // 查询规则执行历史
    QueryResult getRuleExecutionHistory(const std::string& ruleId = "", 
                                       size_t offset = 0, size_t limit = 100);
    QueryResult getRuleExecutionHistory(const std::chrono::system_clock::time_point& start,
                                       const std::chrono::system_clock::time_point& end,
                                       size_t offset = 0, size_t limit = 100);
    
    // 查询传感器数据历史
    QueryResult getSensorDataHistory(const std::string& sensorId = "",
                                    size_t offset = 0, size_t limit = 100);
    QueryResult getSensorDataHistory(const std::string& sensorId,
                                    const std::chrono::system_clock::time_point& start,
                                    const std::chrono::system_clock::time_point& end,
                                    size_t offset = 0, size_t limit = 100);
    
    // 统计信息
    size_t getRuleExecutionCount(const std::string& ruleId = "");
    size_t getSensorDataCount(const std::string& sensorId = "");
    double getRuleSuccessRate(const std::string& ruleId = "");
    double getAverageRuleExecutionTime(const std::string& ruleId = "");
    
    // 清理历史数据
    bool cleanupRuleHistory(const std::chrono::system_clock::time_point& before);
    bool cleanupSensorData(const std::chrono::system_clock::time_point& before);
    bool cleanupByRuleId(const std::string& ruleId);
    bool cleanupBySensorId(const std::string& sensorId);
    
    // 连接管理
    bool connect();
    bool disconnect();
    bool isConnected() const;
    
private:
    std::string generateId() const;
    DataRecord createRuleExecutionDataRecord(const RuleExecutionRecord& record) const;
    DataRecord createSensorDataRecord(const SensorDataRecord& record) const;
};

} // namespace snipper::persistence
