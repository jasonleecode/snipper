#include "bt_node.h"
#include <iostream>

// BTNode 实现
BTNode::BTNode(const string& name, const string& description) 
    : name(name), description(description) {
}

void BTNode::reset() {
    // 默认实现：重置所有子节点
    for (auto& child : children) {
        if (child) {
            child->reset();
        }
    }
}

void BTNode::addChild(shared_ptr<BTNode> child) {
    if (child) {
        child->setParent(shared_from_this());
        children.push_back(child);
    }
}

void BTNode::setParent(shared_ptr<BTNode> parent) {
    this->parent = parent;
}

bool BTNode::isValid() const {
    return !name.empty();
}

json BTNode::getInfo() const {
    json info;
    info["name"] = name;
    info["type"] = getType();
    info["description"] = description;
    info["children_count"] = children.size();
    return info;
}

// BTAction 实现
BTAction::BTAction(const string& name, ActionFunction func, const json& params)
    : BTNode(name, "Action Node"), action_func(func), params(params) {
}

BTStatus BTAction::execute(Context& ctx) {
    if (!action_func) {
        return BTStatus::FAILURE;
    }
    
    try {
        return action_func(ctx);
    } catch (const exception& e) {
        cerr << "Error executing action " << name << ": " << e.what() << endl;
        return BTStatus::FAILURE;
    }
}

string BTAction::getType() const {
    return "Action";
}

// BTCondition 实现
BTCondition::BTCondition(const string& name, ConditionFunction func, const json& params)
    : BTNode(name, "Condition Node"), condition_func(func), params(params) {
}

BTStatus BTCondition::execute(Context& ctx) {
    if (!condition_func) {
        return BTStatus::FAILURE;
    }
    
    try {
        bool result = condition_func(ctx);
        return result ? BTStatus::SUCCESS : BTStatus::FAILURE;
    } catch (const exception& e) {
        cerr << "Error evaluating condition " << name << ": " << e.what() << endl;
        return BTStatus::FAILURE;
    }
}

string BTCondition::getType() const {
    return "Condition";
}

// BTComposite 实现
BTComposite::BTComposite(const string& name, const string& description)
    : BTNode(name, description) {
}

void BTComposite::addChild(shared_ptr<BTNode> child) {
    if (child) {
        child->setParent(shared_from_this());
        children.push_back(child);
    }
}

size_t BTComposite::getChildCount() const {
    return children.size();
}

shared_ptr<BTNode> BTComposite::getChild(size_t index) const {
    if (index < children.size()) {
        return children[index];
    }
    return nullptr;
}

// BTSequence 实现
BTSequence::BTSequence(const string& name)
    : BTComposite(name, "Sequence Node - All children must succeed") {
}

BTStatus BTSequence::execute(Context& ctx) {
    for (auto& child : children) {
        if (!child) continue;
        
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::FAILURE) {
            return BTStatus::FAILURE;
        }
        if (status == BTStatus::RUNNING) {
            return BTStatus::RUNNING;
        }
    }
    return BTStatus::SUCCESS;
}

string BTSequence::getType() const {
    return "Sequence";
}

// BTSelector 实现
BTSelector::BTSelector(const string& name)
    : BTComposite(name, "Selector Node - One child must succeed") {
}

BTStatus BTSelector::execute(Context& ctx) {
    for (auto& child : children) {
        if (!child) continue;
        
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::SUCCESS) {
            return BTStatus::SUCCESS;
        }
        if (status == BTStatus::RUNNING) {
            return BTStatus::RUNNING;
        }
    }
    return BTStatus::FAILURE;
}

string BTSelector::getType() const {
    return "Selector";
}

// BTParallel 实现
BTParallel::BTParallel(const string& name, Policy policy)
    : BTComposite(name, "Parallel Node - Execute children in parallel"), policy(policy) {
}

BTStatus BTParallel::execute(Context& ctx) {
    if (children.empty()) {
        return BTStatus::SUCCESS;
    }
    
    vector<BTStatus> results;
    int success_count = 0;
    int failure_count = 0;
    int running_count = 0;
    
    for (auto& child : children) {
        if (!child) continue;
        
        BTStatus status = child->execute(ctx);
        results.push_back(status);
        
        switch (status) {
            case BTStatus::SUCCESS: success_count++; break;
            case BTStatus::FAILURE: failure_count++; break;
            case BTStatus::RUNNING: running_count++; break;
        }
    }
    
    // 根据策略决定返回状态
    switch (policy) {
        case Policy::SUCCEED_ON_ONE:
            return (success_count > 0) ? BTStatus::SUCCESS : 
                   (running_count > 0) ? BTStatus::RUNNING : BTStatus::FAILURE;
                   
        case Policy::SUCCEED_ON_ALL:
            return (failure_count > 0) ? BTStatus::FAILURE :
                   (running_count > 0) ? BTStatus::RUNNING : BTStatus::SUCCESS;
                   
        case Policy::FAIL_ON_ONE:
            return (failure_count > 0) ? BTStatus::FAILURE :
                   (running_count > 0) ? BTStatus::RUNNING : BTStatus::SUCCESS;
                   
        case Policy::FAIL_ON_ALL:
            return (success_count > 0) ? BTStatus::SUCCESS :
                   (running_count > 0) ? BTStatus::RUNNING : BTStatus::FAILURE;
    }
    
    return BTStatus::FAILURE;
}

string BTParallel::getType() const {
    return "Parallel";
}

// BTDecorator 实现
BTDecorator::BTDecorator(const string& name, const string& description)
    : BTNode(name, description) {
}

void BTDecorator::setChild(shared_ptr<BTNode> child) {
    this->child = child;
    if (child) {
        child->setParent(shared_from_this());
    }
}

void BTDecorator::addChild(shared_ptr<BTNode> child) {
    setChild(child);
}

string BTDecorator::getType() const {
    return "Decorator";
}

// BTInverter 实现
BTInverter::BTInverter(const string& name)
    : BTDecorator(name, "Inverter Node - Invert child result") {
}

BTStatus BTInverter::execute(Context& ctx) {
    if (!child) {
        return BTStatus::FAILURE;
    }
    
    BTStatus status = child->execute(ctx);
    switch (status) {
        case BTStatus::SUCCESS: return BTStatus::FAILURE;
        case BTStatus::FAILURE: return BTStatus::SUCCESS;
        case BTStatus::RUNNING: return BTStatus::RUNNING;
    }
    return BTStatus::FAILURE;
}

string BTInverter::getType() const {
    return "Inverter";
}

// BTRepeater 实现
BTRepeater::BTRepeater(const string& name, int repeat_count)
    : BTDecorator(name, "Repeater Node - Repeat child execution"), 
      repeat_count(repeat_count), current_count(0) {
}

BTStatus BTRepeater::execute(Context& ctx) {
    if (!child) {
        return BTStatus::FAILURE;
    }
    
    // 无限重复
    if (repeat_count == -1) {
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::SUCCESS) {
            child->reset();  // 重置子节点准备下次执行
        }
        return BTStatus::RUNNING;  // 无限重复总是返回RUNNING
    }
    
    // 有限重复
    while (current_count < repeat_count) {
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::SUCCESS) {
            current_count++;
            child->reset();
        } else if (status == BTStatus::FAILURE) {
            return BTStatus::FAILURE;
        } else if (status == BTStatus::RUNNING) {
            return BTStatus::RUNNING;
        }
    }
    
    return BTStatus::SUCCESS;
}

void BTRepeater::reset() {
    BTDecorator::reset();
    current_count = 0;
}

string BTRepeater::getType() const {
    return "Repeater";
}

// BTUntilFail 实现
BTUntilFail::BTUntilFail(const string& name)
    : BTDecorator(name, "UntilFail Node - Repeat until child fails") {
}

BTStatus BTUntilFail::execute(Context& ctx) {
    if (!child) {
        return BTStatus::FAILURE;
    }
    
    while (true) {
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::FAILURE) {
            return BTStatus::SUCCESS;
        }
        if (status == BTStatus::RUNNING) {
            return BTStatus::RUNNING;
        }
        // SUCCESS时继续执行
        child->reset();
    }
}

string BTUntilFail::getType() const {
    return "UntilFail";
}

// BTUntilSuccess 实现
BTUntilSuccess::BTUntilSuccess(const string& name)
    : BTDecorator(name, "UntilSuccess Node - Repeat until child succeeds") {
}

BTStatus BTUntilSuccess::execute(Context& ctx) {
    if (!child) {
        return BTStatus::FAILURE;
    }
    
    while (true) {
        BTStatus status = child->execute(ctx);
        if (status == BTStatus::SUCCESS) {
            return BTStatus::SUCCESS;
        }
        if (status == BTStatus::RUNNING) {
            return BTStatus::RUNNING;
        }
        // FAILURE时继续执行
        child->reset();
    }
}

string BTUntilSuccess::getType() const {
    return "UntilSuccess";
}
