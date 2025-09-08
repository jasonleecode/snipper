#include "engine.h"
#include "../expression/expression.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>

// Engine 实现
void Engine::register_action(const string& name, ActionFn fn) {
    actions_[name] = fn;
}

void Engine::load(const json& cfg) {
    rules_.clear();
    group_manager_ = RuleGroupManager();
    
    if (cfg.contains("rules") && cfg["rules"].is_array()) {
        for (const auto& ruleJson : cfg["rules"]) {
            Rule rule;
            parseRule(ruleJson, rule);
            rules_.push_back(rule);
        }
        
        // 按优先级排序规则
        sort_rules_by_priority();
    }
}

void Engine::onSensorUpdate() {
    // 这里可以添加传感器数据更新的逻辑
    // 例如：从硬件传感器读取数据，更新Context
    // 目前是空实现，实际项目中需要根据具体硬件实现
}

void Engine::tick(Context& ctx) {
    auto now = now_ms();
    for (auto& rule : rules_) {
        if (!rule.shouldExecute(now)) continue;
        
        // 检查规则组状态
        if (!group_manager_.shouldExecuteRule(rule)) continue;
        
        // 检查条件
        if (!rule.condition || !rule.condition->eval(ctx)) continue;
        
        // 执行动作
        for (auto& step : rule.actions) {
            auto it = actions_.find(step.name);
            if (it != actions_.end()) {
                try {
                    it->second(step.params, ctx);
                } catch (const exception& e) {
                    cerr << "Error executing action " << step.name << " in rule " << rule.id << ": " << e.what() << endl;
                }
            } else {
                cerr << "Unknown action: " << step.name << " in rule " << rule.id << endl;
            }
        }
        
        rule.updateLastFire(now);
    }
}

uint64_t Engine::now_ms() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void Engine::sort_rules_by_priority() {
    PriorityManager::sortRules(rules_);
}

void Engine::set_rule_priority(const string& rule_id, int priority) {
    PriorityManager::setRulePriority(rules_, rule_id, priority);
}

void Engine::enable_rule_group(const string& group_name) {
    group_manager_.enableGroup(group_name);
}

void Engine::disable_rule_group(const string& group_name) {
    group_manager_.disableGroup(group_name);
}

void Engine::enable_rule(const string& rule_id) {
    for (auto& rule : rules_) {
        if (rule.id == rule_id) {
            rule.enable();
            return;
        }
    }
}

void Engine::disable_rule(const string& rule_id) {
    for (auto& rule : rules_) {
        if (rule.id == rule_id) {
            rule.disable();
            return;
        }
    }
}

vector<Rule> Engine::get_rules_by_group(const string& group_name) const {
    return group_manager_.getRulesByGroup(rules_, group_name);
}

Rule* Engine::get_rule_by_id(const string& rule_id) {
    for (auto& rule : rules_) {
        if (rule.id == rule_id) {
            return &rule;
        }
    }
    return nullptr;
}

vector<Rule> Engine::get_all_rules() const {
    return rules_;
}

size_t Engine::get_rule_count() const {
    return rules_.size();
}

void Engine::clear_rules() {
    rules_.clear();
}

void Engine::parseRule(const json& ruleJson, Rule& rule) {
    rule.id = ruleJson.value("id", "");
    
    // 解析条件
    if (ruleJson.contains("when")) {
        parseCondition(ruleJson["when"], rule.condition);
    }
    
    // 解析动作
    if (ruleJson.contains("do") && ruleJson["do"].is_array()) {
        for (const auto& actionJson : ruleJson["do"]) {
            ActionStep step;
            step.name = actionJson.value("action", "");
            step.params = actionJson.value("params", json::object());
            rule.actions.push_back(step);
        }
    }
    
    // 解析模式
    string modeStr = ruleJson.value("mode", "repeat");
    rule.mode = (modeStr == "once") ? ONCE : REPEAT;
    
    // 解析节流时间
    rule.throttle_ms = ruleJson.value("throttle_ms", 0);
    
    // 解析优先级
    rule.priority = PriorityManager::normalizePriority(ruleJson.value("priority", 500));
    
    // 解析规则组
    rule.group = ruleJson.value("group", "");
}

void Engine::parseCondition(const json& whenJson, shared_ptr<Condition>& condition) {
    condition = make_shared<Condition>();
    
    if (whenJson.contains("expression")) {
        // 使用表达式条件
        condition->use_expression = true;
        condition->expression = ExpressionParser::parse(whenJson["expression"]);
    } else if (whenJson.contains("all") && whenJson["all"].is_array()) {
        for (const auto& condJson : whenJson["all"]) {
            shared_ptr<Condition> subCond = make_shared<Condition>();
            parseCondition(condJson, subCond);
            condition->all.push_back(subCond);
        }
    } else if (whenJson.contains("any") && whenJson["any"].is_array()) {
        for (const auto& condJson : whenJson["any"]) {
            shared_ptr<Condition> subCond = make_shared<Condition>();
            parseCondition(condJson, subCond);
            condition->any.push_back(subCond);
        }
    } else if (whenJson.contains("left")) {
        condition->left = whenJson.value("left", "");
        condition->op = whenJson.value("op", "");
        condition->right = whenJson["right"];
    }
}
