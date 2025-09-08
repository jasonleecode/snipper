#pragma once

#include "context.h"
#include <string>
#include <vector>
#include <functional>

using namespace std;

// 前向声明
class Condition;

// 动作步骤
struct ActionStep {
    string name;    // 动作名称
    json params;    // 动作参数
};

// 动作函数类型
using ActionFn = function<void(const json& params, Context& ctx)>;

// 规则模式枚举
enum RuleMode {
    ONCE,    // 只执行一次
    REPEAT   // 重复执行
};

// 规则定义
class Rule {
public:
    string id;              // 规则ID
    shared_ptr<Condition> condition;  // 触发条件
    vector<ActionStep> actions;       // 要执行的动作序列
    RuleMode mode;          // 执行模式
    uint64_t throttle_ms;   // 节流时间（毫秒）
    uint64_t last_fire;     // 上次触发时间
    bool disabled;          // 是否已禁用
    int priority;           // 优先级 (0-1000, 越小优先级越高)
    string group;           // 规则组（可选）
    
    Rule();
    
    // 优先级比较函数，用于排序
    bool operator<(const Rule& other) const;
    
    // 检查规则是否应该执行
    bool shouldExecute(uint64_t current_time) const;
    
    // 更新最后执行时间
    void updateLastFire(uint64_t current_time);
    
    // 禁用规则
    void disable();
    
    // 启用规则
    void enable();
};
