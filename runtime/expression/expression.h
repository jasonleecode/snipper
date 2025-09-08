#pragma once

#include "../core/context.h"
#include <string>
#include <vector>
#include <memory>

using namespace std;

// 表达式节点类型
enum ExprType {
    EXPR_VALUE,     // 值节点
    EXPR_VAR,       // 变量节点
    EXPR_OP,        // 操作符节点
    EXPR_FUNC       // 函数节点
};

// 表达式节点
class ExprNode {
public:
    ExprType type;
    string value;           // 值或变量名
    string op;              // 操作符
    string func_name;       // 函数名
    vector<shared_ptr<ExprNode>> children;  // 子节点
    
    ExprNode();
    ExprNode(ExprType t);
    
    // 评估表达式
    Value evaluate(const Context& ctx) const;
    
    // 检查节点是否有效
    bool isValid() const;
};

// 表达式解析器
class ExpressionParser {
public:
    // 解析JSON表达式
    static shared_ptr<ExprNode> parse(const json& expr);
    
    // 解析字符串表达式（未来扩展）
    static shared_ptr<ExprNode> parseString(const string& expr);
    
private:
    // 递归解析JSON
    static shared_ptr<ExprNode> parseRecursive(const json& expr);
};
