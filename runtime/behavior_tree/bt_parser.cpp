#include "bt_parser.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

// BTParser 实现
shared_ptr<BTNode> BTParser::parse(const json& treeJson) {
    if (treeJson.is_null() || !treeJson.contains("root")) {
        return nullptr;
    }
    
    return parseNode(treeJson["root"]);
}

shared_ptr<BTNode> BTParser::parseFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open behavior tree file: " << filename << endl;
        return nullptr;
    }
    
    try {
        json treeJson = json::parse(file);
        return parse(treeJson);
    } catch (const exception& e) {
        cerr << "Error parsing behavior tree file: " << e.what() << endl;
        return nullptr;
    }
}

bool BTParser::validate(const shared_ptr<BTNode>& root) {
    if (!root) {
        return false;
    }
    
    // 检查根节点是否有效
    if (!root->isValid()) {
        return false;
    }
    
    // 递归检查所有子节点
    for (const auto& child : root->children) {
        if (!validate(child)) {
            return false;
        }
    }
    
    return true;
}

json BTParser::getTreeInfo(const shared_ptr<BTNode>& root) {
    if (!root) {
        return json::object();
    }
    
    json info = root->getInfo();
    if (!root->children.empty()) {
        json children_info = json::array();
        for (const auto& child : root->children) {
            children_info.push_back(getTreeInfo(child));
        }
        info["children"] = children_info;
    }
    
    return info;
}

shared_ptr<BTNode> BTParser::parseNode(const json& nodeJson) {
    if (!nodeJson.contains("type")) {
        return nullptr;
    }
    
    string type = nodeJson["type"];
    string name = nodeJson.value("name", "");
    string description = nodeJson.value("description", "");
    
    if (type == "action") {
        return parseAction(nodeJson);
    } else if (type == "condition") {
        return parseCondition(nodeJson);
    } else if (type == "sequence") {
        return parseComposite(nodeJson);
    } else if (type == "selector") {
        return parseComposite(nodeJson);
    } else if (type == "parallel") {
        return parseComposite(nodeJson);
    } else if (type == "inverter") {
        return parseDecorator(nodeJson);
    } else if (type == "repeater") {
        return parseDecorator(nodeJson);
    } else if (type == "until_fail") {
        return parseDecorator(nodeJson);
    } else if (type == "until_success") {
        return parseDecorator(nodeJson);
    }
    
    cerr << "Unknown node type: " << type << endl;
    return nullptr;
}

shared_ptr<BTAction> BTParser::parseAction(const json& nodeJson) {
    string name = nodeJson.value("name", "Action");
    json params = nodeJson.value("params", json::object());
    string action_name = nodeJson.value("action", "");
    
    auto action_func = createDefaultAction(action_name, params);
    return make_shared<BTAction>(name, action_func, params);
}

shared_ptr<BTCondition> BTParser::parseCondition(const json& nodeJson) {
    string name = nodeJson.value("name", "Condition");
    json params = nodeJson.value("params", json::object());
    string condition_name = nodeJson.value("condition", "");
    
    auto condition_func = createDefaultCondition(condition_name, params);
    return make_shared<BTCondition>(name, condition_func, params);
}

shared_ptr<BTComposite> BTParser::parseComposite(const json& nodeJson) {
    string type = nodeJson["type"];
    string name = nodeJson.value("name", type);
    
    shared_ptr<BTComposite> composite;
    
    if (type == "sequence") {
        composite = make_shared<BTSequence>(name);
    } else if (type == "selector") {
        composite = make_shared<BTSelector>(name);
    } else if (type == "parallel") {
        string policy_str = nodeJson.value("policy", "succeed_on_one");
        BTParallel::Policy policy = BTParallel::Policy::SUCCEED_ON_ONE;
        
        if (policy_str == "succeed_on_all") policy = BTParallel::Policy::SUCCEED_ON_ALL;
        else if (policy_str == "fail_on_one") policy = BTParallel::Policy::FAIL_ON_ONE;
        else if (policy_str == "fail_on_all") policy = BTParallel::Policy::FAIL_ON_ALL;
        
        composite = make_shared<BTParallel>(name, policy);
    }
    
    if (composite && nodeJson.contains("children") && nodeJson["children"].is_array()) {
        for (const auto& childJson : nodeJson["children"]) {
            auto child = parseNode(childJson);
            if (child) {
                composite->addChild(child);
            }
        }
    }
    
    return composite;
}

shared_ptr<BTDecorator> BTParser::parseDecorator(const json& nodeJson) {
    string type = nodeJson["type"];
    string name = nodeJson.value("name", type);
    
    shared_ptr<BTDecorator> decorator;
    
    if (type == "inverter") {
        decorator = make_shared<BTInverter>(name);
    } else if (type == "repeater") {
        int repeat_count = nodeJson.value("repeat_count", -1);
        decorator = make_shared<BTRepeater>(name, repeat_count);
    } else if (type == "until_fail") {
        decorator = make_shared<BTUntilFail>(name);
    } else if (type == "until_success") {
        decorator = make_shared<BTUntilSuccess>(name);
    }
    
    if (decorator && nodeJson.contains("child")) {
        auto child = parseNode(nodeJson["child"]);
        if (child) {
            decorator->setChild(child);
        }
    }
    
    return decorator;
}

BTAction::ActionFunction BTParser::createDefaultAction(const string& actionName, const json& params) {
    return [actionName, params](Context& ctx) -> BTStatus {
        // 默认动作实现
        cout << "Executing action: " << actionName << endl;
        
        // 这里可以根据actionName执行不同的动作
        // 实际项目中应该注册具体的动作函数
        
        // 模拟动作执行
        if (actionName == "wait") {
            int duration = params.value("duration", 1000);
            this_thread::sleep_for(chrono::milliseconds(duration));
            return BTStatus::SUCCESS;
        } else if (actionName == "print") {
            string message = params.value("message", "Hello from behavior tree!");
            cout << "Action output: " << message << endl;
            return BTStatus::SUCCESS;
        } else if (actionName == "fail") {
            return BTStatus::FAILURE;
        } else if (actionName == "running") {
            return BTStatus::RUNNING;
        }
        
        return BTStatus::SUCCESS;
    };
}

BTCondition::ConditionFunction BTParser::createDefaultCondition(const string& conditionName, const json& params) {
    return [conditionName, params](Context& ctx) -> bool {
        // 默认条件实现
        cout << "Evaluating condition: " << conditionName << endl;
        
        // 这里可以根据conditionName执行不同的条件检查
        // 实际项目中应该注册具体的条件函数
        
        if (conditionName == "check_value") {
            string key = params.value("key", "");
            string expected = params.value("expected", "");
            return ctx.get(key).get<string>() == expected;
        } else if (conditionName == "check_number") {
            string key = params.value("key", "");
            double threshold = params.value("threshold", 0.0);
            string op = params.value("operator", ">");
            
            double value = ctx.get(key).get<double>();
            if (op == ">") return value > threshold;
            if (op == "<") return value < threshold;
            if (op == "==") return value == threshold;
            if (op == ">=") return value >= threshold;
            if (op == "<=") return value <= threshold;
        } else if (conditionName == "always_true") {
            return true;
        } else if (conditionName == "always_false") {
            return false;
        }
        
        return true;
    };
}
