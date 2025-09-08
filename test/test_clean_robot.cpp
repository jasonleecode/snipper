#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 清洁机器人任务测试 ===" << endl;
    
    // 创建行为树管理器
    BTManager btManager;
    
    // 注册清洁机器人专用动作函数
    btManager.registerAction("turn_around", [](Context& ctx) -> BTStatus {
        cout << "  🔄 执行调头动作..." << endl;
        // 模拟调头动作
        this_thread::sleep_for(chrono::milliseconds(1000));
        cout << "  ✅ 调头完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("move_forward", [](Context& ctx) -> BTStatus {
        cout << "  ➡️ 向前移动..." << endl;
        // 模拟移动
        this_thread::sleep_for(chrono::milliseconds(500));
        cout << "  ✅ 移动完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("rotate_360", [](Context& ctx) -> BTStatus {
        cout << "  🔄 旋转360度进行定位..." << endl;
        // 模拟旋转定位
        this_thread::sleep_for(chrono::milliseconds(2000));
        cout << "  ✅ 定位完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("clean_area", [](Context& ctx) -> BTStatus {
        cout << "  🧹 清扫当前区域..." << endl;
        // 模拟清扫
        this_thread::sleep_for(chrono::milliseconds(2000));
        cout << "  ✅ 区域清扫完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("stop_cleaning", [](Context& ctx) -> BTStatus {
        cout << "  ⏹️ 停止清扫..." << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("navigate_to", [](Context& ctx) -> BTStatus {
        cout << "  🧭 导航到充电站..." << endl;
        // 模拟导航
        this_thread::sleep_for(chrono::milliseconds(1500));
        cout << "  ✅ 到达充电站" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("start_charging", [](Context& ctx) -> BTStatus {
        cout << "  🔌 开始充电..." << endl;
        // 模拟充电过程
        this_thread::sleep_for(chrono::milliseconds(3000));
        cout << "  ✅ 充电完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("wait_condition", [](Context& ctx) -> BTStatus {
        cout << "  ⏳ 等待条件满足..." << endl;
        return BTStatus::SUCCESS;
    });
    
    btManager.registerAction("set_value", [](Context& ctx) -> BTStatus {
        cout << "  🔧 设置上下文值..." << endl;
        return BTStatus::SUCCESS;
    });
    
    // 注册条件函数
    btManager.registerCondition("check_number", [](Context& ctx) -> bool {
        // 简化实现：总是返回true
        return true;
    });
    
    btManager.registerCondition("check_value", [](Context& ctx) -> bool {
        // 简化实现：总是返回true
        return true;
    });
    
    // 加载清洁机器人任务
    cout << "\n1. 加载清洁机器人任务..." << endl;
    string filename = "tasks/clean_robot_task.json";
    if (!btManager.loadTree("clean_robot", filename)) {
        cerr << "Failed to load clean robot task" << endl;
        return 1;
    }
    
    cout << "清洁机器人任务加载成功！" << endl;
    
    // 创建上下文
    Context ctx;
    
    // 测试场景1：开机定位（电量充足，未完成定位）
    cout << "\n2. 测试场景1：开机定位" << endl;
    cout << "设置上下文：电量=85%, 清扫状态=进行中, 定位完成=否" << endl;
    ctx.set("battery_level", 85.0);
    ctx.set("cleaning_status", "in_progress");
    ctx.set("wall_collision", "false");
    ctx.set("need_charging", "false");
    ctx.set("positioning_completed", "false");
    
    cout << "执行清洁任务..." << endl;
    BTStatus status = btManager.executeTree("clean_robot", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景2：正常清扫（电量充足，无碰撞，已完成定位）
    cout << "\n3. 测试场景2：正常清扫" << endl;
    cout << "设置上下文：电量=85%, 清扫状态=进行中, 墙壁碰撞=否, 定位完成=是" << endl;
    ctx.set("battery_level", 85.0);
    ctx.set("cleaning_status", "in_progress");
    ctx.set("wall_collision", "false");
    ctx.set("need_charging", "false");
    ctx.set("positioning_completed", "true");
    
    cout << "执行清洁任务..." << endl;
    status = btManager.executeTree("clean_robot", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景3：墙壁碰撞
    cout << "\n4. 测试场景3：墙壁碰撞" << endl;
    cout << "设置上下文：电量=60%, 清扫状态=进行中, 墙壁碰撞=是, 定位完成=是" << endl;
    ctx.set("battery_level", 60.0);
    ctx.set("cleaning_status", "in_progress");
    ctx.set("wall_collision", "true");
    ctx.set("need_charging", "false");
    ctx.set("positioning_completed", "true");
    
    cout << "执行清洁任务..." << endl;
    status = btManager.executeTree("clean_robot", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景4：电量不足，需要充电
    cout << "\n5. 测试场景4：电量不足" << endl;
    cout << "设置上下文：电量=3%, 清扫状态=进行中, 墙壁碰撞=否, 需要充电=是, 定位完成=是" << endl;
    ctx.set("battery_level", 3.0);
    ctx.set("cleaning_status", "in_progress");
    ctx.set("wall_collision", "false");
    ctx.set("need_charging", "true");
    ctx.set("positioning_completed", "true");
    
    cout << "执行清洁任务..." << endl;
    status = btManager.executeTree("clean_robot", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 显示行为树信息
    cout << "\n6. 清洁机器人行为树信息:" << endl;
    json treeInfo = btManager.getTreeInfo("clean_robot");
    cout << treeInfo.dump(2) << endl;
    
    // 显示执行统计
    cout << "\n7. 执行统计:" << endl;
    json stats = btManager.getExecutionStats("clean_robot");
    cout << stats.dump(2) << endl;
    
    cout << "\n=== 清洁机器人任务测试完成 ===" << endl;
    return 0;
}
