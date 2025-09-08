#pragma once

#include "../core/context.h"
#include <string>
#include <vector>
#include <memory>

using namespace std;

// 条件类型
class Condition {
public:
    // 简单条件（向后兼容）
    string left;    // 左操作数（通常是传感器名称）
    string op;      // 操作符 (">", "==", "<", ">=", "<=", "!=")
    Value right;    // 右操作数（比较值）
    
    // 复合条件
    vector<shared_ptr<Condition>> all;  // 所有条件都必须满足
    vector<shared_ptr<Condition>> any;  // 任一条件满足即可
    
    // 表达式条件（新功能）
    shared_ptr<class ExprNode> expression;    // 表达式树
    bool use_expression;    // 是否使用表达式
    
    Condition();
    
    // 评估条件
    bool eval(const Context& ctx) const;
    
    // 检查是否为空条件
    bool isEmpty() const;
};

