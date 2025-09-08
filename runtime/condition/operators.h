#pragma once

#include "../core/context.h"
#include <string>

using namespace std;

// 条件评估器
struct Eval {
    // 基本比较操作
    static bool cmp(const Value& a, const string& op, const Value& b);
    
    // 数学运算
    static Value add(const Value& a, const Value& b);
    static Value subtract(const Value& a, const Value& b);
    static Value multiply(const Value& a, const Value& b);
    static Value divide(const Value& a, const Value& b);
    static Value modulo(const Value& a, const Value& b);
    
    // 逻辑运算
    static Value logical_and(const Value& a, const Value& b);
    static Value logical_or(const Value& a, const Value& b);
    static Value logical_not(const Value& a);
    
    // 字符串操作
    static Value string_contains(const Value& str, const Value& substr);
    static Value string_starts_with(const Value& str, const Value& prefix);
    static Value string_ends_with(const Value& str, const Value& suffix);
    
    // 时间操作
    static Value time_between(const Value& time, const Value& start, const Value& end);
    static Value day_of_week(const Value& time);
    
    // 历史数据操作（简化实现）
    static Value avg_last_n(const Context& ctx, const string& var, int n);
    static Value max_last_n(const Context& ctx, const string& var, int n);
    static Value trend(const Context& ctx, const string& var, int n);
};
