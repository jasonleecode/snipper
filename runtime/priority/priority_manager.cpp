#include "priority_manager.h"
#include <algorithm>

// PriorityManager 实现
void PriorityManager::sortRules(vector<Rule>& rules) {
    sort(rules.begin(), rules.end());
}

void PriorityManager::setRulePriority(vector<Rule>& rules, const string& rule_id, int priority) {
    for (auto& rule : rules) {
        if (rule.id == rule_id) {
            rule.priority = normalizePriority(priority);
            sortRules(rules);
            return;
        }
    }
}

int PriorityManager::getRulePriority(const vector<Rule>& rules, const string& rule_id) {
    for (const auto& rule : rules) {
        if (rule.id == rule_id) {
            return rule.priority;
        }
    }
    return 500; // 默认优先级
}

bool PriorityManager::isValidPriority(int priority) {
    return priority >= 0 && priority <= 1000;
}

int PriorityManager::normalizePriority(int priority) {
    if (priority < 0) return 0;
    if (priority > 1000) return 1000;
    return priority;
}

// RuleGroupManager 实现
void RuleGroupManager::enableGroup(const string& group_name) {
    group_states_[group_name] = true;
}

void RuleGroupManager::disableGroup(const string& group_name) {
    group_states_[group_name] = false;
}

bool RuleGroupManager::isGroupEnabled(const string& group_name) const {
    auto it = group_states_.find(group_name);
    return it != group_states_.end() ? it->second : true; // 默认启用
}

vector<Rule> RuleGroupManager::getRulesByGroup(const vector<Rule>& rules, const string& group_name) const {
    vector<Rule> result;
    for (const auto& rule : rules) {
        if (rule.group == group_name) {
            result.push_back(rule);
        }
    }
    return result;
}

bool RuleGroupManager::shouldExecuteRule(const Rule& rule) const {
    if (rule.group.empty()) return true; // 没有组的规则总是执行
    return isGroupEnabled(rule.group);
}
