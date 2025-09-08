#pragma once

// 数据持久化和状态管理模块统一头文件

#include "storage_interface.h"
#include "memory_storage.h"
#include "file_storage.h"
#include "history_recorder.h"
#include "rule_state_manager.h"
#include "config_hot_reload.h"

namespace snipper::persistence {

// 持久化管理器 - 统一管理所有持久化功能
class PersistenceManager {
private:
    std::unique_ptr<StorageInterface> storage_;
    std::unique_ptr<HistoryRecorder> historyRecorder_;
    std::unique_ptr<RuleStateManager> ruleStateManager_;
    std::unique_ptr<ConfigHotReload> configHotReload_;
    
public:
    explicit PersistenceManager(std::unique_ptr<StorageInterface> storage);
    ~PersistenceManager() = default;
    
    // 连接管理
    bool connect();
    bool disconnect();
    bool isConnected() const;
    
    // 存储访问
    StorageInterface* getStorage() const;
    HistoryRecorder* getHistoryRecorder() const;
    RuleStateManager* getRuleStateManager() const;
    ConfigHotReload* getConfigHotReload() const;
    
    // 便捷方法
    bool recordRuleExecution(const std::string& ruleId, const std::string& ruleName, 
                           bool success, const std::string& errorMessage = "");
    bool recordSensorData(const std::string& sensorId, const std::string& sensorType,
                         const json& value, const std::string& unit = "");
    bool updateRuleState(const std::string& ruleId, RuleState state);
    bool addConfigFile(const std::string& configPath);
    
    // 统计信息
    json getSystemStats() const;
    json getRuleStats(const std::string& ruleId = "") const;
    json getSensorStats(const std::string& sensorId = "") const;
};

} // namespace snipper::persistence
