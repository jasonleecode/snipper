#pragma once

#include "context.h"
#include "rule.h"
#include "../condition/condition_evaluator.h"
#include "../priority/priority_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

using namespace std;

// 规则引擎
class Engine {
public:
    // 注册动作函数
    void register_action(const string& name, ActionFn fn);
    
    // 加载规则配置
    void load(const json& cfg);
    
    // 传感器数据更新
    void onSensorUpdate();
    
    // 执行规则检查
    void tick(Context& ctx);
    
    // 获取当前时间（毫秒）
    static uint64_t now_ms();
    
    // 规则优先级管理
    void sort_rules_by_priority();
    void set_rule_priority(const string& rule_id, int priority);
    void enable_rule_group(const string& group_name);
    void disable_rule_group(const string& group_name);
    void enable_rule(const string& rule_id);
    void disable_rule(const string& rule_id);
    
    // 规则查询
    vector<Rule> get_rules_by_group(const string& group_name) const;
    Rule* get_rule_by_id(const string& rule_id);
    vector<Rule> get_all_rules() const;
    
    // 获取规则数量
    size_t get_rule_count() const;
    
    // 清空所有规则
    void clear_rules();
    
private:
    vector<Rule> rules_;
    unordered_map<string, ActionFn> actions_;
    RuleGroupManager group_manager_;
    
    // 解析规则配置
    void parseRule(const json& ruleJson, Rule& rule);
    void parseCondition(const json& whenJson, shared_ptr<Condition>& condition);
};
