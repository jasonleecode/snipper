#pragma once

#include "storage_interface.h"
#include "history_recorder.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace snipper::persistence {

using json = nlohmann::json;

// 规则状态
enum class RuleState {
    DISABLED,    // 禁用
    ENABLED,     // 启用
    RUNNING,     // 运行中
    PAUSED,      // 暂停
    ERROR        // 错误
};

// 规则状态信息
struct RuleStateInfo {
    std::string ruleId;
    std::string ruleName;
    RuleState state;
    json config;
    json context;
    std::chrono::system_clock::time_point lastUpdate;
    std::chrono::system_clock::time_point lastExecution;
    int executionCount;
    int errorCount;
    std::string lastError;
    
    RuleStateInfo() = default;
    RuleStateInfo(const std::string& ruleId, const std::string& ruleName, 
                 RuleState state = RuleState::ENABLED)
        : ruleId(ruleId), ruleName(ruleName), state(state), 
          lastUpdate(std::chrono::system_clock::now()),
          executionCount(0), errorCount(0) {}
};

// 规则状态管理器
class RuleStateManager {
private:
    std::unique_ptr<StorageInterface> storage_;
    std::unique_ptr<HistoryRecorder> historyRecorder_;
    std::unordered_map<std::string, RuleStateInfo> ruleStates_;
    mutable std::mutex mutex_;
    std::string source_;
    
public:
    explicit RuleStateManager(std::unique_ptr<StorageInterface> storage,
                            const std::string& source = "snipper");
    ~RuleStateManager() = default;
    
    // 规则状态管理
    bool registerRule(const std::string& ruleId, const std::string& ruleName, 
                     const json& config = json::object());
    bool unregisterRule(const std::string& ruleId);
    bool updateRuleState(const std::string& ruleId, RuleState state);
    bool updateRuleConfig(const std::string& ruleId, const json& config);
    bool updateRuleContext(const std::string& ruleId, const json& context);
    
    // 规则执行跟踪
    bool recordRuleStart(const std::string& ruleId, const json& context = json::object());
    bool recordRuleEnd(const std::string& ruleId, bool success, 
                      const std::string& errorMessage = "");
    bool recordRuleError(const std::string& ruleId, const std::string& errorMessage);
    
    // 查询规则状态
    RuleStateInfo getRuleState(const std::string& ruleId) const;
    std::vector<RuleStateInfo> getAllRuleStates() const;
    std::vector<RuleStateInfo> getRulesByState(RuleState state) const;
    bool isRuleEnabled(const std::string& ruleId) const;
    bool isRuleRunning(const std::string& ruleId) const;
    
    // 规则统计
    size_t getTotalRuleCount() const;
    size_t getEnabledRuleCount() const;
    size_t getRunningRuleCount() const;
    size_t getErrorRuleCount() const;
    double getRuleSuccessRate(const std::string& ruleId) const;
    double getOverallSuccessRate() const;
    
    // 持久化操作
    bool saveRuleStates();
    bool loadRuleStates();
    bool saveRuleState(const std::string& ruleId);
    bool loadRuleState(const std::string& ruleId);
    
    // 清理操作
    bool cleanupRuleHistory(const std::chrono::system_clock::time_point& before);
    bool resetRuleStats(const std::string& ruleId);
    bool resetAllRuleStats();
    
    // 连接管理
    bool connect();
    bool disconnect();
    bool isConnected() const;
    
    // 历史记录访问
    HistoryRecorder* getHistoryRecorder() const;
    
private:
    std::string generateId() const;
    DataRecord createRuleStateDataRecord(const RuleStateInfo& stateInfo) const;
    RuleStateInfo parseRuleStateFromRecord(const DataRecord& record) const;
    std::string ruleStateToString(RuleState state) const;
    RuleState stringToRuleState(const std::string& stateStr) const;
};

} // namespace snipper::persistence
