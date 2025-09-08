#include "operators.h"
#include <cmath>
#include <ctime>

// Eval 实现
bool Eval::cmp(const Value& a, const string& op, const Value& b) {
    if (op == "==") return a == b;
    if (op == "!=") return a != b;
    if (op == ">") return a > b;
    if (op == "<") return a < b;
    if (op == ">=") return a >= b;
    if (op == "<=") return a <= b;
    return false;
}

// 数学运算
Value Eval::add(const Value& a, const Value& b) {
    if (a.is_number() && b.is_number()) {
        return a.get<double>() + b.get<double>();
    }
    if (a.is_string() && b.is_string()) {
        return a.get<string>() + b.get<string>();
    }
    return Value();
}

Value Eval::subtract(const Value& a, const Value& b) {
    if (a.is_number() && b.is_number()) {
        return a.get<double>() - b.get<double>();
    }
    return Value();
}

Value Eval::multiply(const Value& a, const Value& b) {
    if (a.is_number() && b.is_number()) {
        return a.get<double>() * b.get<double>();
    }
    return Value();
}

Value Eval::divide(const Value& a, const Value& b) {
    if (a.is_number() && b.is_number()) {
        double b_val = b.get<double>();
        if (b_val != 0) {
            return a.get<double>() / b_val;
        }
    }
    return Value();
}

Value Eval::modulo(const Value& a, const Value& b) {
    if (a.is_number() && b.is_number()) {
        double b_val = b.get<double>();
        if (b_val != 0) {
            return fmod(a.get<double>(), b_val);
        }
    }
    return Value();
}

// 逻辑运算
Value Eval::logical_and(const Value& a, const Value& b) {
    bool a_val = a.is_boolean() ? a.get<bool>() : (a.is_number() ? a.get<double>() != 0 : !a.is_null());
    bool b_val = b.is_boolean() ? b.get<bool>() : (b.is_number() ? b.get<double>() != 0 : !b.is_null());
    return a_val && b_val;
}

Value Eval::logical_or(const Value& a, const Value& b) {
    bool a_val = a.is_boolean() ? a.get<bool>() : (a.is_number() ? a.get<double>() != 0 : !a.is_null());
    bool b_val = b.is_boolean() ? b.get<bool>() : (b.is_number() ? b.get<double>() != 0 : !b.is_null());
    return a_val || b_val;
}

Value Eval::logical_not(const Value& a) {
    bool a_val = a.is_boolean() ? a.get<bool>() : (a.is_number() ? a.get<double>() != 0 : !a.is_null());
    return !a_val;
}

// 字符串操作
Value Eval::string_contains(const Value& str, const Value& substr) {
    if (str.is_string() && substr.is_string()) {
        return str.get<string>().find(substr.get<string>()) != string::npos;
    }
    return false;
}

Value Eval::string_starts_with(const Value& str, const Value& prefix) {
    if (str.is_string() && prefix.is_string()) {
        const string& s = str.get<string>();
        const string& p = prefix.get<string>();
        return s.length() >= p.length() && s.substr(0, p.length()) == p;
    }
    return false;
}

Value Eval::string_ends_with(const Value& str, const Value& suffix) {
    if (str.is_string() && suffix.is_string()) {
        const string& s = str.get<string>();
        const string& suf = suffix.get<string>();
        return s.length() >= suf.length() && s.substr(s.length() - suf.length()) == suf;
    }
    return false;
}

// 时间操作
Value Eval::time_between(const Value& time, const Value& start, const Value& end) {
    // 简化实现，实际项目中需要更复杂的时间解析
    if (time.is_string() && start.is_string() && end.is_string()) {
        // 这里应该解析时间字符串并比较
        // 暂时返回false作为占位符
        return false;
    }
    return false;
}

Value Eval::day_of_week(const Value& time) {
    // 简化实现，返回当前星期几
    if (time.is_string()) {
        time_t now = std::time(0);
        struct tm* timeinfo = localtime(&now);
        return timeinfo->tm_wday; // 0=Sunday, 1=Monday, etc.
    }
    return -1;
}

// 历史数据操作（简化实现）
Value Eval::avg_last_n(const Context& ctx, const string& var, int n) {
    // 这里应该从历史数据中计算平均值
    // 暂时返回当前值作为占位符
    return ctx.get(var);
}

Value Eval::max_last_n(const Context& ctx, const string& var, int n) {
    // 这里应该从历史数据中找最大值
    // 暂时返回当前值作为占位符
    return ctx.get(var);
}

Value Eval::trend(const Context& ctx, const string& var, int n) {
    // 这里应该计算趋势（上升/下降/平稳）
    // 暂时返回0作为占位符
    return 0;
}
