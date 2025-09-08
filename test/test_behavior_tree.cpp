#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 行为树功能测试 ===" << endl;
    
    // 创建行为树管理器
    BTManager btManager;
    
    // 注册自定义动作函数
    btManager.registerAction("move_forward", [](Context& ctx) -> BTStatus {
        cout << "  → 向前移动" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("turn_left", [](Context& ctx) -> BTStatus {
        cout << "  ↺ 向左转" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("turn_right", [](Context& ctx) -> BTStatus {
        cout << "  ↻ 向右转" << endl;
        return BTStatus::SUCCESS;
    });
    
    // 注册自定义条件函数
    btManager.registerCondition("is_obstacle_ahead", [](Context& ctx) -> bool {
        bool obstacle = ctx.get("obstacle_ahead").get<bool>();
        cout << "  ? 前方有障碍物: " << (obstacle ? "是" : "否") << endl;
        return obstacle;
    });
    
    btManager.registerCondition("is_target_reached", [](Context& ctx) -> bool {
        bool reached = ctx.get("target_reached").get<bool>();
        cout << "  ? 到达目标: " << (reached ? "是" : "否") << endl;
        return reached;
    });
    
    // 加载行为树
    cout << "\n1. 加载行为树..." << endl;
    string filename = "tasks/behavior_tree_example.json";
    if (!btManager.loadTree("robot_behavior", filename)) {
        cerr << "Failed to load behavior tree" << endl;
        return 1;
    }
    
    cout << "行为树加载成功！" << endl;
    
    // 创建上下文
    Context ctx;
    
    // 测试场景1：电池充足，房间脏，需要清洁
    cout << "\n2. 测试场景1：清洁任务" << endl;
    cout << "设置上下文：电池=80%, 房间状态=脏" << endl;
    ctx.set("battery_level", 80.0);
    ctx.set("room_status", "dirty");
    
    cout << "执行行为树..." << endl;
    BTStatus status = btManager.executeTree("robot_behavior", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景2：电池充足，房间干净，需要巡逻
    cout << "\n3. 测试场景2：巡逻任务" << endl;
    cout << "设置上下文：电池=60%, 房间状态=干净" << endl;
    ctx.set("battery_level", 60.0);
    ctx.set("room_status", "clean");
    
    cout << "执行行为树..." << endl;
    status = btManager.executeTree("robot_behavior", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景3：电池不足，应该失败
    cout << "\n4. 测试场景3：电池不足" << endl;
    cout << "设置上下文：电池=15%, 房间状态=脏" << endl;
    ctx.set("battery_level", 15.0);
    ctx.set("room_status", "dirty");
    
    cout << "执行行为树..." << endl;
    status = btManager.executeTree("robot_behavior", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 显示行为树信息
    cout << "\n5. 行为树信息:" << endl;
    json treeInfo = btManager.getTreeInfo("robot_behavior");
    cout << treeInfo.dump(2) << endl;
    
    // 显示执行统计
    cout << "\n6. 执行统计:" << endl;
    json stats = btManager.getExecutionStats("robot_behavior");
    cout << stats.dump(2) << endl;
    
    cout << "\n=== 行为树测试完成 ===" << endl;
    return 0;
}
