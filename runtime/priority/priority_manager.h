#pragma once

#include "../core/rule.h"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// 优先级管理器
class PriorityManager {
public:
    // 按优先级排序规则
    static void sortRules(vector<Rule>& rules);
    
    // 设置规则优先级
    static void setRulePriority(vector<Rule>& rules, const string& rule_id, int priority);
    
    // 获取规则优先级
    static int getRulePriority(const vector<Rule>& rules, const string& rule_id);
    
    // 验证优先级范围
    static bool isValidPriority(int priority);
    
    // 标准化优先级
    static int normalizePriority(int priority);
};

// 规则组管理器
class RuleGroupManager {
public:
    // 启用规则组
    void enableGroup(const string& group_name);
    
    // 禁用规则组
    void disableGroup(const string& group_name);
    
    // 检查规则组是否启用
    bool isGroupEnabled(const string& group_name) const;
    
    // 获取规则组中的规则
    vector<Rule> getRulesByGroup(const vector<Rule>& rules, const string& group_name) const;
    
    // 检查规则是否应该执行（考虑组状态）
    bool shouldExecuteRule(const Rule& rule) const;
    
private:
    unordered_map<string, bool> group_states_;
};
