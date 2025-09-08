#include "expression.h"
#include "../condition/operators.h"
#include <iostream>

// ExprNode 实现
ExprNode::ExprNode() : type(EXPR_VALUE) {
}

ExprNode::ExprNode(ExprType t) : type(t) {
}

Value ExprNode::evaluate(const Context& ctx) const {
    switch (type) {
        case EXPR_VALUE:
            return Value(value);
            
        case EXPR_VAR:
            return ctx.get(value);
            
        case EXPR_OP: {
            if (children.size() < 2) return Value();
            
            Value left = children[0]->evaluate(ctx);
            Value right = children[1]->evaluate(ctx);
            
            if (op == "+") return Eval::add(left, right);
            if (op == "-") return Eval::subtract(left, right);
            if (op == "*") return Eval::multiply(left, right);
            if (op == "/") return Eval::divide(left, right);
            if (op == "%") return Eval::modulo(left, right);
            if (op == "&&") return Eval::logical_and(left, right);
            if (op == "||") return Eval::logical_or(left, right);
            if (op == "==") return left == right;
            if (op == "!=") return left != right;
            if (op == ">") return left > right;
            if (op == "<") return left < right;
            if (op == ">=") return left >= right;
            if (op == "<=") return left <= right;
            
            return Value();
        }
        
        case EXPR_FUNC: {
            if (children.empty()) return Value();
            
            if (func_name == "contains") {
                if (children.size() >= 2) {
                    return Eval::string_contains(children[0]->evaluate(ctx), children[1]->evaluate(ctx));
                }
            } else if (func_name == "starts_with") {
                if (children.size() >= 2) {
                    return Eval::string_starts_with(children[0]->evaluate(ctx), children[1]->evaluate(ctx));
                }
            } else if (func_name == "ends_with") {
                if (children.size() >= 2) {
                    return Eval::string_ends_with(children[0]->evaluate(ctx), children[1]->evaluate(ctx));
                }
            } else if (func_name == "time_between") {
                if (children.size() >= 3) {
                    return Eval::time_between(children[0]->evaluate(ctx), children[1]->evaluate(ctx), children[2]->evaluate(ctx));
                }
            } else if (func_name == "day_of_week") {
                if (children.size() >= 1) {
                    return Eval::day_of_week(children[0]->evaluate(ctx));
                }
            } else if (func_name == "avg_last_n") {
                if (children.size() >= 2) {
                    string var = children[0]->evaluate(ctx).get<string>();
                    int n = children[1]->evaluate(ctx).get<int>();
                    return Eval::avg_last_n(ctx, var, n);
                }
            } else if (func_name == "max_last_n") {
                if (children.size() >= 2) {
                    string var = children[0]->evaluate(ctx).get<string>();
                    int n = children[1]->evaluate(ctx).get<int>();
                    return Eval::max_last_n(ctx, var, n);
                }
            } else if (func_name == "trend") {
                if (children.size() >= 2) {
                    string var = children[0]->evaluate(ctx).get<string>();
                    int n = children[1]->evaluate(ctx).get<int>();
                    return Eval::trend(ctx, var, n);
                }
            }
            
            return Value();
        }
        
        default:
            return Value();
    }
}

bool ExprNode::isValid() const {
    return type != EXPR_VALUE || !value.empty();
}

// ExpressionParser 实现
shared_ptr<ExprNode> ExpressionParser::parse(const json& expr) {
    return parseRecursive(expr);
}

shared_ptr<ExprNode> ExpressionParser::parseString(const string& expr) {
    // 未来可以实现字符串表达式解析
    // 目前返回空指针
    return nullptr;
}

shared_ptr<ExprNode> ExpressionParser::parseRecursive(const json& expr) {
    auto node = make_shared<ExprNode>();
    
    if (expr.is_string()) {
        node->type = EXPR_VAR;
        node->value = expr.get<string>();
    } else if (expr.is_number() || expr.is_boolean()) {
        node->type = EXPR_VALUE;
        node->value = expr.dump();
    } else if (expr.is_object()) {
        if (expr.contains("op")) {
            node->type = EXPR_OP;
            node->op = expr["op"];
            if (expr.contains("left")) {
                node->children.push_back(parseRecursive(expr["left"]));
            }
            if (expr.contains("right")) {
                node->children.push_back(parseRecursive(expr["right"]));
            }
        } else if (expr.contains("func")) {
            node->type = EXPR_FUNC;
            node->func_name = expr["func"];
            if (expr.contains("args") && expr["args"].is_array()) {
                for (const auto& arg : expr["args"]) {
                    node->children.push_back(parseRecursive(arg));
                }
            }
        } else {
            // 如果不是操作符或函数，可能是简单的值
            node->type = EXPR_VALUE;
            node->value = expr.dump();
        }
    }
    
    return node;
}
