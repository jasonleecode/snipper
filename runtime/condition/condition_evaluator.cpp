#include "condition_evaluator.h"
#include "operators.h"
#include "../expression/expression.h"

// Condition 实现
Condition::Condition() : use_expression(false) {
}

bool Condition::eval(const Context& ctx) const {
    // 使用表达式评估
    if (use_expression && expression) {
        Value result = expression->evaluate(ctx);
        return result.is_boolean() ? result.get<bool>() : false;
    }
    
    // 处理复合条件
    if (!all.empty()) {
        for (const auto& cond : all) {
            if (!cond || !cond->eval(ctx)) return false;
        }
        return true;
    }
    
    if (!any.empty()) {
        for (const auto& cond : any) {
            if (cond && cond->eval(ctx)) return true;
        }
        return false;
    }
    
    // 处理简单条件
    Value leftValue = ctx.get(left);
    return Eval::cmp(leftValue, op, right);
}

bool Condition::isEmpty() const {
    return !use_expression && 
           left.empty() && 
           all.empty() && 
           any.empty();
}
