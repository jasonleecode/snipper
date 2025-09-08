#include "bt_manager.h"
#include <iostream>

// BTManager 实现
BTManager::BTManager() {
}

bool BTManager::loadTree(const string& name, const string& filename) {
    auto root = BTParser::parseFromFile(filename);
    if (!root) {
        cerr << "Failed to load behavior tree from file: " << filename << endl;
        return false;
    }
    
    auto executor = make_shared<BTExecutor>();
    executor->setRoot(root);
    trees_[name] = executor;
    
    cout << "Loaded behavior tree: " << name << endl;
    return true;
}

bool BTManager::loadTree(const string& name, const json& treeJson) {
    auto root = BTParser::parse(treeJson);
    if (!root) {
        cerr << "Failed to parse behavior tree: " << name << endl;
        return false;
    }
    
    auto executor = make_shared<BTExecutor>();
    executor->setRoot(root);
    trees_[name] = executor;
    
    cout << "Loaded behavior tree: " << name << endl;
    return true;
}

BTStatus BTManager::executeTree(const string& name, Context& ctx) {
    auto executor = getExecutor(name);
    if (!executor) {
        cerr << "Behavior tree not found: " << name << endl;
        return BTStatus::FAILURE;
    }
    
    return executor->execute(ctx);
}

void BTManager::stopTree(const string& name) {
    auto executor = getExecutor(name);
    if (executor) {
        executor->stop();
    }
}

void BTManager::resetTree(const string& name) {
    auto executor = getExecutor(name);
    if (executor) {
        executor->reset();
    }
}

void BTManager::pauseTree(const string& name) {
    auto executor = getExecutor(name);
    if (executor) {
        executor->pause();
    }
}

void BTManager::resumeTree(const string& name) {
    auto executor = getExecutor(name);
    if (executor) {
        executor->resume();
    }
}

BTStatus BTManager::getTreeStatus(const string& name) const {
    auto executor = getExecutor(name);
    if (!executor) {
        return BTStatus::FAILURE;
    }
    
    return executor->getStatus();
}

bool BTManager::hasTree(const string& name) const {
    return trees_.find(name) != trees_.end();
}

vector<string> BTManager::getTreeNames() const {
    vector<string> names;
    for (const auto& pair : trees_) {
        names.push_back(pair.first);
    }
    return names;
}

void BTManager::removeTree(const string& name) {
    auto it = trees_.find(name);
    if (it != trees_.end()) {
        trees_.erase(it);
        cout << "Removed behavior tree: " << name << endl;
    }
}

void BTManager::clear() {
    trees_.clear();
    cout << "Cleared all behavior trees" << endl;
}

void BTManager::registerAction(const string& name, BTAction::ActionFunction func) {
    // 为所有执行器注册动作
    for (auto& pair : trees_) {
        pair.second->registerAction(name, func);
    }
}

void BTManager::registerCondition(const string& name, BTCondition::ConditionFunction func) {
    // 为所有执行器注册条件
    for (auto& pair : trees_) {
        pair.second->registerCondition(name, func);
    }
}

json BTManager::getTreeInfo(const string& name) const {
    auto executor = getExecutor(name);
    if (!executor) {
        return json::object();
    }
    
    return executor->getTreeInfo();
}

json BTManager::getAllTreesInfo() const {
    json info;
    for (const auto& pair : trees_) {
        info[pair.first] = pair.second->getTreeInfo();
    }
    return info;
}

json BTManager::getExecutionStats(const string& name) const {
    auto executor = getExecutor(name);
    if (!executor) {
        return json::object();
    }
    
    return executor->getExecutionStats();
}

json BTManager::getAllExecutionStats() const {
    json stats;
    for (const auto& pair : trees_) {
        stats[pair.first] = pair.second->getExecutionStats();
    }
    return stats;
}

shared_ptr<BTExecutor> BTManager::getExecutor(const string& name) const {
    auto it = trees_.find(name);
    return (it != trees_.end()) ? it->second : nullptr;
}
