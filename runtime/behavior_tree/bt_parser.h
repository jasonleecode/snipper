#pragma once

#include "bt_node.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

// 行为树解析器
class BTParser {
public:
    // 从JSON解析行为树
    static shared_ptr<BTNode> parse(const json& treeJson);
    
    // 从文件解析行为树
    static shared_ptr<BTNode> parseFromFile(const string& filename);
    
    // 验证行为树结构
    static bool validate(const shared_ptr<BTNode>& root);
    
    // 获取行为树信息
    static json getTreeInfo(const shared_ptr<BTNode>& root);
    
private:
    // 递归解析节点
    static shared_ptr<BTNode> parseNode(const json& nodeJson);
    
    // 解析动作节点
    static shared_ptr<BTAction> parseAction(const json& nodeJson);
    
    // 解析条件节点
    static shared_ptr<BTCondition> parseCondition(const json& nodeJson);
    
    // 解析组合节点
    static shared_ptr<BTComposite> parseComposite(const json& nodeJson);
    
    // 解析装饰器节点
    static shared_ptr<BTDecorator> parseDecorator(const json& nodeJson);
    
    // 创建默认动作函数
    static BTAction::ActionFunction createDefaultAction(const string& actionName, const json& params);
    
    // 创建默认条件函数
    static BTCondition::ConditionFunction createDefaultCondition(const string& conditionName, const json& params);
};
