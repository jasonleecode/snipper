#pragma once

#include "bt_executor.h"
#include "bt_parser.h"
#include "../core/context.h"
#include <string>
#include <unordered_map>
#include <memory>

using namespace std;

// 行为树管理器
class BTManager {
public:
    // 构造函数
    BTManager();
    
    // 加载行为树
    bool loadTree(const string& name, const string& filename);
    bool loadTree(const string& name, const json& treeJson);
    
    // 执行行为树
    BTStatus executeTree(const string& name, Context& ctx);
    
    // 停止行为树
    void stopTree(const string& name);
    
    // 重置行为树
    void resetTree(const string& name);
    
    // 暂停行为树
    void pauseTree(const string& name);
    
    // 恢复行为树
    void resumeTree(const string& name);
    
    // 获取行为树状态
    BTStatus getTreeStatus(const string& name) const;
    
    // 检查行为树是否存在
    bool hasTree(const string& name) const;
    
    // 获取所有行为树名称
    vector<string> getTreeNames() const;
    
    // 移除行为树
    void removeTree(const string& name);
    
    // 清空所有行为树
    void clear();
    
    // 注册动作函数
    void registerAction(const string& name, BTAction::ActionFunction func);
    
    // 注册条件函数
    void registerCondition(const string& name, BTCondition::ConditionFunction func);
    
    // 获取行为树信息
    json getTreeInfo(const string& name) const;
    
    // 获取所有行为树信息
    json getAllTreesInfo() const;
    
    // 获取执行统计
    json getExecutionStats(const string& name) const;
    
    // 获取所有执行统计
    json getAllExecutionStats() const;
    
private:
    unordered_map<string, shared_ptr<BTExecutor>> trees_;
    
    // 获取执行器
    shared_ptr<BTExecutor> getExecutor(const string& name) const;
};
