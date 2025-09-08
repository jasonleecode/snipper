#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 多条件数据回传任务测试 ===" << endl;
    
    // 创建行为树管理器
    BTManager btManager;
    
    // 注册数据回传动作函数
    btManager.registerAction("data_upload", [](Context& ctx) -> BTStatus {
        cout << "  📡 开始数据回传..." << endl;
        cout << "  📊 回传类型: 车辆遥测数据" << endl;
        cout << "  ⏱️ 回传时长: 30秒" << endl;
        cout << "  🔒 数据压缩: 已启用" << endl;
        cout << "  📈 优先级: 高" << endl;
        
        // 模拟数据回传过程
        for (int i = 1; i <= 30; i++) {
            this_thread::sleep_for(chrono::milliseconds(100));
            if (i % 5 == 0) {
                cout << "  📤 回传进度: " << i << "/30 秒" << endl;
            }
        }
        
        cout << "  ✅ 数据回传完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    // 注册条件函数
    btManager.registerCondition("check_value", [](Context& ctx) -> bool {
        // 简化实现：总是返回true
        return true;
    });
    
    btManager.registerCondition("check_number", [](Context& ctx) -> bool {
        // 简化实现：总是返回true
        return true;
    });
    
    // 加载多条件任务
    cout << "\n1. 加载多条件数据回传任务..." << endl;
    string filename = "tasks/multi_condition_task.json";
    if (!btManager.loadTree("multi_condition", filename)) {
        cerr << "Failed to load multi condition task" << endl;
        return 1;
    }
    
    cout << "多条件任务加载成功！" << endl;
    
    // 创建上下文
    Context ctx;
    
    // 测试场景1：所有条件都满足
    cout << "\n2. 测试场景1：所有条件满足" << endl;
    cout << "设置上下文：" << endl;
    cout << "  - 城市位置: 杭州" << endl;
    cout << "  - 车速: 85km/h" << endl;
    cout << "  - 天气: 雷雨天" << endl;
    cout << "  - 路况: 下高速匝道" << endl;
    
    ctx.set("city_location", "hangzhou");
    ctx.set("vehicle_speed", 85.0);
    ctx.set("weather_condition", "thunderstorm");
    ctx.set("road_scenario", "highway_exit_ramp");
    ctx.set("upload_status", "idle");
    
    cout << "\n执行多条件任务..." << endl;
    BTStatus status = btManager.executeTree("multi_condition", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景2：城市位置不满足
    cout << "\n3. 测试场景2：城市位置不满足" << endl;
    cout << "设置上下文：" << endl;
    cout << "  - 城市位置: 上海 (不满足)" << endl;
    cout << "  - 车速: 85km/h" << endl;
    cout << "  - 天气: 雷雨天" << endl;
    cout << "  - 路况: 下高速匝道" << endl;
    
    ctx.set("city_location", "shanghai");
    ctx.set("vehicle_speed", 85.0);
    ctx.set("weather_condition", "thunderstorm");
    ctx.set("road_scenario", "highway_exit_ramp");
    
    cout << "\n执行多条件任务..." << endl;
    status = btManager.executeTree("multi_condition", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景3：车速不满足
    cout << "\n4. 测试场景3：车速不满足" << endl;
    cout << "设置上下文：" << endl;
    cout << "  - 城市位置: 杭州" << endl;
    cout << "  - 车速: 60km/h (不满足)" << endl;
    cout << "  - 天气: 雷雨天" << endl;
    cout << "  - 路况: 下高速匝道" << endl;
    
    ctx.set("city_location", "hangzhou");
    ctx.set("vehicle_speed", 60.0);
    ctx.set("weather_condition", "thunderstorm");
    ctx.set("road_scenario", "highway_exit_ramp");
    
    cout << "\n执行多条件任务..." << endl;
    status = btManager.executeTree("multi_condition", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景4：天气条件不满足
    cout << "\n5. 测试场景4：天气条件不满足" << endl;
    cout << "设置上下文：" << endl;
    cout << "  - 城市位置: 杭州" << endl;
    cout << "  - 车速: 85km/h" << endl;
    cout << "  - 天气: 晴天 (不满足)" << endl;
    cout << "  - 路况: 下高速匝道" << endl;
    
    ctx.set("city_location", "hangzhou");
    ctx.set("vehicle_speed", 85.0);
    ctx.set("weather_condition", "sunny");
    ctx.set("road_scenario", "highway_exit_ramp");
    
    cout << "\n执行多条件任务..." << endl;
    status = btManager.executeTree("multi_condition", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景5：路况条件不满足
    cout << "\n6. 测试场景5：路况条件不满足" << endl;
    cout << "设置上下文：" << endl;
    cout << "  - 城市位置: 杭州" << endl;
    cout << "  - 车速: 85km/h" << endl;
    cout << "  - 天气: 雷雨天" << endl;
    cout << "  - 路况: 城市道路 (不满足)" << endl;
    
    ctx.set("city_location", "hangzhou");
    ctx.set("vehicle_speed", 85.0);
    ctx.set("weather_condition", "thunderstorm");
    ctx.set("road_scenario", "city_road");
    
    cout << "\n执行多条件任务..." << endl;
    status = btManager.executeTree("multi_condition", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 显示行为树信息
    cout << "\n7. 多条件任务行为树信息:" << endl;
    json treeInfo = btManager.getTreeInfo("multi_condition");
    cout << treeInfo.dump(2) << endl;
    
    // 显示执行统计
    cout << "\n8. 执行统计:" << endl;
    json stats = btManager.getExecutionStats("multi_condition");
    cout << stats.dump(2) << endl;
    
    cout << "\n=== 多条件数据回传任务测试完成 ===" << endl;
    return 0;
}
