#include "persistence.h"

namespace snipper::persistence {

PersistenceManager::PersistenceManager(std::unique_ptr<StorageInterface> storage)
    : storage_(std::move(storage)) {
    
    if (storage_) {
        // 创建历史记录器（使用相同的存储）
        auto historyStorage = StorageFactory::create("memory", nlohmann::json::object());
        historyRecorder_ = std::make_unique<HistoryRecorder>(std::move(historyStorage), "snipper");
        
        // 创建规则状态管理器
        auto stateStorage = StorageFactory::create("memory", nlohmann::json::object());
        ruleStateManager_ = std::make_unique<RuleStateManager>(std::move(stateStorage), "snipper");
        
        // 创建配置热重载管理器
        configHotReload_ = std::make_unique<ConfigHotReload>();
    }
}

bool PersistenceManager::connect() {
    bool success = true;
    
    if (storage_) {
        success = success && storage_->connect();
    }
    
    if (historyRecorder_) {
        success = success && historyRecorder_->connect();
    }
    
    if (ruleStateManager_) {
        success = success && ruleStateManager_->connect();
    }
    
    return success;
}

bool PersistenceManager::disconnect() {
    bool success = true;
    
    if (configHotReload_) {
        configHotReload_->stopMonitoring();
    }
    
    if (ruleStateManager_) {
        success = success && ruleStateManager_->disconnect();
    }
    
    if (historyRecorder_) {
        success = success && historyRecorder_->disconnect();
    }
    
    if (storage_) {
        success = success && storage_->disconnect();
    }
    
    return success;
}

bool PersistenceManager::isConnected() const {
    return storage_ && storage_->isConnected();
}

StorageInterface* PersistenceManager::getStorage() const {
    return storage_.get();
}

HistoryRecorder* PersistenceManager::getHistoryRecorder() const {
    return historyRecorder_.get();
}

RuleStateManager* PersistenceManager::getRuleStateManager() const {
    return ruleStateManager_.get();
}

ConfigHotReload* PersistenceManager::getConfigHotReload() const {
    return configHotReload_.get();
}

bool PersistenceManager::recordRuleExecution(const std::string& ruleId, const std::string& ruleName, 
                                           bool success, const std::string& errorMessage) {
    if (!historyRecorder_) {
        return false;
    }
    
    return historyRecorder_->recordRuleEnd(ruleId, success, errorMessage);
}

bool PersistenceManager::recordSensorData(const std::string& sensorId, const std::string& sensorType,
                                         const json& value, const std::string& unit) {
    if (!historyRecorder_) {
        return false;
    }
    
    return historyRecorder_->recordSensorData(sensorId, sensorType, value, unit);
}

bool PersistenceManager::updateRuleState(const std::string& ruleId, RuleState state) {
    if (!ruleStateManager_) {
        return false;
    }
    
    return ruleStateManager_->updateRuleState(ruleId, state);
}

bool PersistenceManager::addConfigFile(const std::string& configPath) {
    if (!configHotReload_) {
        return false;
    }
    
    return configHotReload_->addConfigFile(configPath);
}

json PersistenceManager::getSystemStats() const {
    json stats;
    
    if (ruleStateManager_) {
        stats["rules"] = {
            {"total", ruleStateManager_->getTotalRuleCount()},
            {"enabled", ruleStateManager_->getEnabledRuleCount()},
            {"running", ruleStateManager_->getRunningRuleCount()},
            {"error", ruleStateManager_->getErrorRuleCount()},
            {"success_rate", ruleStateManager_->getOverallSuccessRate()}
        };
    }
    
    if (historyRecorder_) {
        stats["history"] = {
            {"rule_executions", historyRecorder_->getRuleExecutionCount()},
            {"sensor_data_points", historyRecorder_->getSensorDataCount()}
        };
    }
    
    if (configHotReload_) {
        auto configPaths = configHotReload_->getConfigPaths();
        stats["config"] = {
            {"monitored_files", configPaths.size()},
            {"files", configPaths}
        };
    }
    
    return stats;
}

json PersistenceManager::getRuleStats(const std::string& ruleId) const {
    json stats;
    
    if (ruleStateManager_) {
        auto ruleState = ruleStateManager_->getRuleState(ruleId);
        if (!ruleState.ruleId.empty()) {
            stats["rule_id"] = ruleState.ruleId;
            stats["rule_name"] = ruleState.ruleName;
            stats["state"] = ruleState.state;
            stats["execution_count"] = ruleState.executionCount;
            stats["error_count"] = ruleState.errorCount;
            stats["last_error"] = ruleState.lastError;
            stats["last_execution"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                ruleState.lastExecution.time_since_epoch()).count();
        }
    }
    
    if (historyRecorder_) {
        stats["success_rate"] = historyRecorder_->getRuleSuccessRate(ruleId);
        stats["avg_execution_time"] = historyRecorder_->getAverageRuleExecutionTime(ruleId);
    }
    
    return stats;
}

json PersistenceManager::getSensorStats(const std::string& sensorId) const {
    json stats;
    
    if (historyRecorder_) {
        stats["data_count"] = historyRecorder_->getSensorDataCount(sensorId);
    }
    
    return stats;
}

} // namespace snipper::persistence
