#include "rule.h"
#include "../condition/condition_evaluator.h"

// Rule 实现
Rule::Rule() 
    : throttle_ms(0), last_fire(0), disabled(false), priority(500) {
}

bool Rule::operator<(const Rule& other) const {
    if (priority != other.priority) {
        return priority < other.priority;  // 优先级小的在前
    }
    return id < other.id;  // 相同优先级按ID字典序
}

bool Rule::shouldExecute(uint64_t current_time) const {
    if (disabled) return false;
    if (current_time - last_fire < throttle_ms) return false;
    return true;
}

void Rule::updateLastFire(uint64_t current_time) {
    last_fire = current_time;
    if (mode == ONCE) {
        disabled = true;
    }
}

void Rule::disable() {
    disabled = true;
}

void Rule::enable() {
    disabled = false;
}
