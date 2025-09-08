#pragma once

#include "../core/context.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

using namespace std;

// 行为树节点执行状态
enum class BTStatus {
    SUCCESS,    // 成功
    FAILURE,    // 失败
    RUNNING     // 运行中
};

// 行为树节点基类
class BTNode : public std::enable_shared_from_this<BTNode> {
public:
    string name;                    // 节点名称
    string description;             // 节点描述
    vector<shared_ptr<BTNode>> children;  // 子节点
    shared_ptr<BTNode> parent;     // 父节点
    
    BTNode(const string& name = "", const string& description = "");
    virtual ~BTNode() = default;
    
    // 执行节点
    virtual BTStatus execute(Context& ctx) = 0;
    
    // 重置节点状态
    virtual void reset();
    
    // 添加子节点
    void addChild(shared_ptr<BTNode> child);
    
    // 设置父节点
    void setParent(shared_ptr<BTNode> parent);
    
    // 获取节点类型
    virtual string getType() const = 0;
    
    // 检查节点是否有效
    virtual bool isValid() const;
    
    // 获取节点信息
    virtual json getInfo() const;
};

// 动作节点（叶子节点）
class BTAction : public BTNode {
public:
    using ActionFunction = function<BTStatus(Context&)>;
    
    ActionFunction action_func;     // 动作函数
    json params;                   // 动作参数
    
    BTAction(const string& name, ActionFunction func, const json& params = json::object());
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 条件节点（叶子节点）
class BTCondition : public BTNode {
public:
    using ConditionFunction = function<bool(Context&)>;
    
    ConditionFunction condition_func;  // 条件函数
    json params;                      // 条件参数
    
    BTCondition(const string& name, ConditionFunction func, const json& params = json::object());
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 组合节点基类
class BTComposite : public BTNode {
public:
    BTComposite(const string& name, const string& description = "");
    
    // 添加子节点
    virtual void addChild(shared_ptr<BTNode> child);
    
    // 获取子节点数量
    size_t getChildCount() const;
    
    // 获取子节点
    shared_ptr<BTNode> getChild(size_t index) const;
};

// 顺序执行节点（所有子节点必须成功）
class BTSequence : public BTComposite {
public:
    BTSequence(const string& name = "Sequence");
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 选择执行节点（任一子节点成功即可）
class BTSelector : public BTComposite {
public:
    BTSelector(const string& name = "Selector");
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 并行执行节点
class BTParallel : public BTComposite {
public:
    enum class Policy {
        SUCCEED_ON_ONE,     // 一个成功即成功
        SUCCEED_ON_ALL,     // 全部成功才成功
        FAIL_ON_ONE,        // 一个失败即失败
        FAIL_ON_ALL         // 全部失败才失败
    };
    
    Policy policy;
    
    BTParallel(const string& name = "Parallel", Policy policy = Policy::SUCCEED_ON_ONE);
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 装饰器节点基类
class BTDecorator : public BTNode {
public:
    shared_ptr<BTNode> child;  // 装饰的子节点
    
    BTDecorator(const string& name, const string& description = "");
    
    // 设置子节点
    void setChild(shared_ptr<BTNode> child);
    
    // 添加子节点（装饰器只能有一个子节点）
    virtual void addChild(shared_ptr<BTNode> child);
    
    string getType() const override;
};

// 反转器节点（成功变失败，失败变成功）
class BTInverter : public BTDecorator {
public:
    BTInverter(const string& name = "Inverter");
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 重复器节点
class BTRepeater : public BTDecorator {
public:
    int repeat_count;      // 重复次数（-1表示无限重复）
    int current_count;     // 当前计数
    
    BTRepeater(const string& name = "Repeater", int repeat_count = -1);
    
    BTStatus execute(Context& ctx) override;
    void reset() override;
    string getType() const override;
};

// 直到失败节点（重复执行直到失败）
class BTUntilFail : public BTDecorator {
public:
    BTUntilFail(const string& name = "UntilFail");
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};

// 直到成功节点（重复执行直到成功）
class BTUntilSuccess : public BTDecorator {
public:
    BTUntilSuccess(const string& name = "UntilSuccess");
    
    BTStatus execute(Context& ctx) override;
    string getType() const override;
};
