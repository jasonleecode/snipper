#include "rule_state_manager.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace snipper::persistence {

RuleStateManager::RuleStateManager(std::unique_ptr<StorageInterface> storage,
                                  const std::string& source)
    : storage_(std::move(storage)), source_(source) {
    if (storage_) {
        auto historyStorage = StorageFactory::create("memory", nlohmann::json::object());
        historyRecorder_ = std::make_unique<HistoryRecorder>(std::move(historyStorage), source_);
    }
}

bool RuleStateManager::registerRule(const std::string& ruleId, const std::string& ruleName, 
                                   const json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    RuleStateInfo stateInfo(ruleId, ruleName, RuleState::ENABLED);
    stateInfo.config = config;
    stateInfo.lastUpdate = std::chrono::system_clock::now();
    
    ruleStates_[ruleId] = stateInfo;
    
    // 持久化到存储
    return saveRuleState(ruleId);
}

bool RuleStateManager::unregisterRule(const std::string& ruleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        ruleStates_.erase(it);
        
        // 从存储中删除
        std::vector<QueryCondition> conditions;
        conditions.emplace_back("type", "==", "rule_state");
        conditions.emplace_back("rule_id", "==", ruleId);
        
        QueryResult result = storage_->query(conditions, 0, 1);
        if (!result.records.empty()) {
            storage_->remove(result.records[0].id);
        }
        
        return true;
    }
    
    return false;
}

bool RuleStateManager::updateRuleState(const std::string& ruleId, RuleState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.state = state;
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        return saveRuleState(ruleId);
    }
    
    return false;
}

bool RuleStateManager::updateRuleConfig(const std::string& ruleId, const json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.config = config;
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        return saveRuleState(ruleId);
    }
    
    return false;
}

bool RuleStateManager::updateRuleContext(const std::string& ruleId, const json& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.context = context;
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        return saveRuleState(ruleId);
    }
    
    return false;
}

bool RuleStateManager::recordRuleStart(const std::string& ruleId, const json& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.state = RuleState::RUNNING;
        it->second.lastExecution = std::chrono::system_clock::now();
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        saveRuleState(ruleId);
    }
    
    // 记录到历史
    if (historyRecorder_) {
        return historyRecorder_->recordRuleStart(ruleId, it->second.ruleName, context);
    }
    
    return true;
}

bool RuleStateManager::recordRuleEnd(const std::string& ruleId, bool success, 
                                    const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.state = success ? RuleState::ENABLED : RuleState::ERROR;
        it->second.executionCount++;
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        if (!success) {
            it->second.errorCount++;
            it->second.lastError = errorMessage;
        }
        
        saveRuleState(ruleId);
    }
    
    // 记录到历史
    if (historyRecorder_) {
        return historyRecorder_->recordRuleEnd(ruleId, success, errorMessage);
    }
    
    return true;
}

bool RuleStateManager::recordRuleError(const std::string& ruleId, const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.state = RuleState::ERROR;
        it->second.errorCount++;
        it->second.lastError = errorMessage;
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        saveRuleState(ruleId);
    }
    
    // 记录到历史
    if (historyRecorder_) {
        return historyRecorder_->recordRuleEnd(ruleId, false, errorMessage);
    }
    
    return true;
}

RuleStateInfo RuleStateManager::getRuleState(const std::string& ruleId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        return it->second;
    }
    
    return RuleStateInfo();
}

std::vector<RuleStateInfo> RuleStateManager::getAllRuleStates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RuleStateInfo> states;
    states.reserve(ruleStates_.size());
    
    for (const auto& pair : ruleStates_) {
        states.push_back(pair.second);
    }
    
    return states;
}

std::vector<RuleStateInfo> RuleStateManager::getRulesByState(RuleState state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RuleStateInfo> states;
    
    for (const auto& pair : ruleStates_) {
        if (pair.second.state == state) {
            states.push_back(pair.second);
        }
    }
    
    return states;
}

bool RuleStateManager::isRuleEnabled(const std::string& ruleId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = ruleStates_.find(ruleId);
    return it != ruleStates_.end() && it->second.state == RuleState::ENABLED;
}

bool RuleStateManager::isRuleRunning(const std::string& ruleId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = ruleStates_.find(ruleId);
    return it != ruleStates_.end() && it->second.state == RuleState::RUNNING;
}

size_t RuleStateManager::getTotalRuleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ruleStates_.size();
}

size_t RuleStateManager::getEnabledRuleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& pair : ruleStates_) {
        if (pair.second.state == RuleState::ENABLED) {
            count++;
        }
    }
    
    return count;
}

size_t RuleStateManager::getRunningRuleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& pair : ruleStates_) {
        if (pair.second.state == RuleState::RUNNING) {
            count++;
        }
    }
    
    return count;
}

size_t RuleStateManager::getErrorRuleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& pair : ruleStates_) {
        if (pair.second.state == RuleState::ERROR) {
            count++;
        }
    }
    
    return count;
}

double RuleStateManager::getRuleSuccessRate(const std::string& ruleId) const {
    if (historyRecorder_) {
        return historyRecorder_->getRuleSuccessRate(ruleId);
    }
    return 0.0;
}

double RuleStateManager::getOverallSuccessRate() const {
    if (historyRecorder_) {
        // 计算所有规则的平均成功率
        auto allStates = getAllRuleStates();
        if (allStates.empty()) {
            return 0.0;
        }
        
        double totalRate = 0.0;
        size_t validRules = 0;
        
        for (const auto& state : allStates) {
            double rate = historyRecorder_->getRuleSuccessRate(state.ruleId);
            if (rate > 0.0) {
                totalRate += rate;
                validRules++;
            }
        }
        
        return validRules > 0 ? totalRate / validRules : 0.0;
    }
    
    return 0.0;
}

bool RuleStateManager::saveRuleStates() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    bool success = true;
    for (const auto& pair : ruleStates_) {
        if (!saveRuleState(pair.first)) {
            success = false;
        }
    }
    
    return success;
}

bool RuleStateManager::loadRuleStates() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!storage_ || !storage_->isConnected()) {
        return false;
    }
    
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_state");
    
    QueryResult result = storage_->query(conditions, 0, 10000);
    
    ruleStates_.clear();
    
    for (const auto& record : result.records) {
        RuleStateInfo stateInfo = parseRuleStateFromRecord(record);
        if (!stateInfo.ruleId.empty()) {
            ruleStates_[stateInfo.ruleId] = stateInfo;
        }
    }
    
    return true;
}

bool RuleStateManager::saveRuleState(const std::string& ruleId) {
    auto it = ruleStates_.find(ruleId);
    if (it == ruleStates_.end()) {
        return false;
    }
    
    DataRecord dataRecord = createRuleStateDataRecord(it->second);
    
    // 检查是否已存在
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_state");
    conditions.emplace_back("rule_id", "==", ruleId);
    
    QueryResult result = storage_->query(conditions, 0, 1);
    
    if (result.records.empty()) {
        return storage_->insert(dataRecord);
    } else {
        return storage_->update(result.records[0].id, dataRecord.data);
    }
}

bool RuleStateManager::loadRuleState(const std::string& ruleId) {
    std::vector<QueryCondition> conditions;
    conditions.emplace_back("type", "==", "rule_state");
    conditions.emplace_back("rule_id", "==", ruleId);
    
    QueryResult result = storage_->query(conditions, 0, 1);
    
    if (!result.records.empty()) {
        RuleStateInfo stateInfo = parseRuleStateFromRecord(result.records[0]);
        if (!stateInfo.ruleId.empty()) {
            ruleStates_[ruleId] = stateInfo;
            return true;
        }
    }
    
    return false;
}

bool RuleStateManager::cleanupRuleHistory(const std::chrono::system_clock::time_point& before) {
    if (historyRecorder_) {
        return historyRecorder_->cleanupRuleHistory(before);
    }
    return false;
}

bool RuleStateManager::resetRuleStats(const std::string& ruleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = ruleStates_.find(ruleId);
    if (it != ruleStates_.end()) {
        it->second.executionCount = 0;
        it->second.errorCount = 0;
        it->second.lastError = "";
        it->second.lastUpdate = std::chrono::system_clock::now();
        
        return saveRuleState(ruleId);
    }
    
    return false;
}

bool RuleStateManager::resetAllRuleStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool success = true;
    for (auto& pair : ruleStates_) {
        pair.second.executionCount = 0;
        pair.second.errorCount = 0;
        pair.second.lastError = "";
        pair.second.lastUpdate = std::chrono::system_clock::now();
        
        if (!saveRuleState(pair.first)) {
            success = false;
        }
    }
    
    return success;
}

bool RuleStateManager::connect() {
    if (!storage_) {
        return false;
    }
    
    bool success = storage_->connect();
    if (success && historyRecorder_) {
        historyRecorder_->connect();
    }
    
    return success;
}

bool RuleStateManager::disconnect() {
    if (!storage_) {
        return false;
    }
    
    bool success = storage_->disconnect();
    if (historyRecorder_) {
        historyRecorder_->disconnect();
    }
    
    return success;
}

bool RuleStateManager::isConnected() const {
    return storage_ && storage_->isConnected();
}

HistoryRecorder* RuleStateManager::getHistoryRecorder() const {
    return historyRecorder_.get();
}

std::string RuleStateManager::generateId() const {
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

DataRecord RuleStateManager::createRuleStateDataRecord(const RuleStateInfo& stateInfo) const {
    DataRecord dataRecord;
    dataRecord.id = generateId();
    dataRecord.type = "rule_state";
    dataRecord.source = source_;
    dataRecord.timestamp = stateInfo.lastUpdate;
    
    json data;
    data["rule_id"] = stateInfo.ruleId;
    data["rule_name"] = stateInfo.ruleName;
    data["state"] = ruleStateToString(stateInfo.state);
    data["config"] = stateInfo.config;
    data["context"] = stateInfo.context;
    data["last_update"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        stateInfo.lastUpdate.time_since_epoch()).count();
    data["last_execution"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        stateInfo.lastExecution.time_since_epoch()).count();
    data["execution_count"] = stateInfo.executionCount;
    data["error_count"] = stateInfo.errorCount;
    data["last_error"] = stateInfo.lastError;
    
    dataRecord.data = data;
    return dataRecord;
}

RuleStateInfo RuleStateManager::parseRuleStateFromRecord(const DataRecord& record) const {
    RuleStateInfo stateInfo;
    
    if (record.data.contains("rule_id")) {
        stateInfo.ruleId = record.data["rule_id"].get<std::string>();
    }
    if (record.data.contains("rule_name")) {
        stateInfo.ruleName = record.data["rule_name"].get<std::string>();
    }
    if (record.data.contains("state")) {
        stateInfo.state = stringToRuleState(record.data["state"].get<std::string>());
    }
    if (record.data.contains("config")) {
        stateInfo.config = record.data["config"];
    }
    if (record.data.contains("context")) {
        stateInfo.context = record.data["context"];
    }
    if (record.data.contains("last_update")) {
        auto timestampMs = record.data["last_update"].get<int64_t>();
        stateInfo.lastUpdate = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestampMs));
    }
    if (record.data.contains("last_execution")) {
        auto timestampMs = record.data["last_execution"].get<int64_t>();
        stateInfo.lastExecution = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestampMs));
    }
    if (record.data.contains("execution_count")) {
        stateInfo.executionCount = record.data["execution_count"].get<int>();
    }
    if (record.data.contains("error_count")) {
        stateInfo.errorCount = record.data["error_count"].get<int>();
    }
    if (record.data.contains("last_error")) {
        stateInfo.lastError = record.data["last_error"].get<std::string>();
    }
    
    return stateInfo;
}

std::string RuleStateManager::ruleStateToString(RuleState state) const {
    switch (state) {
        case RuleState::DISABLED: return "disabled";
        case RuleState::ENABLED: return "enabled";
        case RuleState::RUNNING: return "running";
        case RuleState::PAUSED: return "paused";
        case RuleState::ERROR: return "error";
        default: return "unknown";
    }
}

RuleState RuleStateManager::stringToRuleState(const std::string& stateStr) const {
    if (stateStr == "disabled") return RuleState::DISABLED;
    if (stateStr == "enabled") return RuleState::ENABLED;
    if (stateStr == "running") return RuleState::RUNNING;
    if (stateStr == "paused") return RuleState::PAUSED;
    if (stateStr == "error") return RuleState::ERROR;
    return RuleState::ENABLED; // 默认值
}

} // namespace snipper::persistence
