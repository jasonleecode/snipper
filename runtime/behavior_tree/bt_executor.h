#pragma once

#include "bt_node.h"
#include "../core/context.h"
#include <memory>
#include <unordered_map>
#include <functional>

using namespace std;

// 行为树执行器
class BTExecutor {
public:
    // 构造函数
    BTExecutor();
    
    // 设置根节点
    void setRoot(shared_ptr<BTNode> root);
    
    // 执行行为树
    BTStatus execute(Context& ctx);
    
    // 重置行为树
    void reset();
    
    // 暂停执行
    void pause();
    
    // 恢复执行
    void resume();
    
    // 停止执行
    void stop();
    
    // 检查是否正在运行
    bool isRunning() const;
    
    // 获取执行状态
    BTStatus getStatus() const;
    
    // 注册动作函数
    void registerAction(const string& name, BTAction::ActionFunction func);
    
    // 注册条件函数
    void registerCondition(const string& name, BTCondition::ConditionFunction func);
    
    // 获取行为树信息
    json getTreeInfo() const;
    
    // 获取执行统计
    json getExecutionStats() const;
    
private:
    shared_ptr<BTNode> root_;
    BTStatus current_status_;
    bool is_running_;
    bool is_paused_;
    
    // 动作和条件函数注册表
    unordered_map<string, BTAction::ActionFunction> action_functions_;
    unordered_map<string, BTCondition::ConditionFunction> condition_functions_;
    
    // 执行统计
    int execution_count_;
    int success_count_;
    int failure_count_;
    int running_count_;
    
    // 更新统计
    void updateStats(BTStatus status);
};
