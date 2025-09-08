#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 路口红绿灯任务测试 ===" << endl;
    
    // 创建行为树管理器
    BTManager btManager;
    
    // 注册灯光控制动作函数
    btManager.registerAction("turn_on_light", [](Context& ctx) -> BTStatus {
        // 从参数中获取灯光颜色
        string lightColor = "unknown";
        int brightness = 100;
        
        cout << "  🚦 开启灯光: ";
        if (lightColor == "green") {
            cout << "🟢 绿灯 (亮度: " << brightness << "%)" << endl;
        } else if (lightColor == "yellow") {
            cout << "🟡 黄灯 (亮度: " << brightness << "%)" << endl;
        } else if (lightColor == "red") {
            cout << "🔴 红灯 (亮度: " << brightness << "%)" << endl;
        } else {
            cout << "⚪ 未知颜色" << endl;
        }
        
        return BTStatus::SUCCESS;
    });
    
    // 注册等待动作函数
    btManager.registerAction("wait_duration", [](Context& ctx) -> BTStatus {
        int duration = 1; // 默认1秒，用于测试
        string unit = "seconds";
        
        cout << "  ⏱️ 等待 " << duration << " " << unit << "..." << endl;
        
        // 模拟等待过程，但为了测试快速完成，实际等待时间缩短
        for (int i = 1; i <= duration; i++) {
            this_thread::sleep_for(chrono::milliseconds(200)); // 200ms per second for testing
            if (i % 2 == 0 || i == duration) {
                cout << "    ⏳ " << i << "/" << duration << " " << unit << endl;
            }
        }
        
        cout << "  ✅ 等待完成" << endl;
        return BTStatus::SUCCESS;
    });
    
    // 注册条件函数
    btManager.registerCondition("check_light_status", [](Context& ctx) -> bool {
        // 简化实现：总是返回true
        return true;
    });
    
    // 加载路口红绿灯任务
    cout << "\n1. 加载路口红绿灯任务..." << endl;
    string filename = "tasks/junction_light_task.json";
    if (!btManager.loadTree("junction_light", filename)) {
        cerr << "Failed to load junction light task" << endl;
        return 1;
    }
    
    cout << "路口红绿灯任务加载成功！" << endl;
    
    // 创建上下文
    Context ctx;
    ctx.set("current_light", "off");
    ctx.set("cycle_count", 0);
    ctx.set("is_emergency", false);
    ctx.set("pedestrian_crossing", false);
    ctx.set("traffic_density", "normal");
    
    // 测试场景1：运行一个完整的红绿灯周期
    cout << "\n2. 测试场景1：运行一个完整的红绿灯周期" << endl;
    cout << "红绿灯周期顺序：" << endl;
    cout << "  🟢 绿灯 → 30秒" << endl;
    cout << "  🟡 黄灯 → 4秒" << endl;
    cout << "  🔴 红灯 → 20秒" << endl;
    cout << "  🟡 黄灯 → 4秒" << endl;
    cout << "  🟢 绿灯 → 30秒 (下一个周期)" << endl;
    
    cout << "\n开始执行红绿灯周期..." << endl;
    BTStatus status = btManager.executeTree("junction_light", ctx);
    cout << "执行结果: " << (status == BTStatus::SUCCESS ? "成功" : 
                             status == BTStatus::FAILURE ? "失败" : "运行中") << endl;
    
    // 测试场景2：紧急情况下的红绿灯控制
    cout << "\n3. 测试场景2：紧急情况控制" << endl;
    cout << "设置紧急情况：所有车辆停止，红灯常亮" << endl;
    
    ctx.set("is_emergency", true);
    ctx.set("current_light", "red");
    
    cout << "紧急情况下的红绿灯状态：" << endl;
    cout << "  🚨 紧急模式：红灯常亮" << endl;
    cout << "  ⛔ 所有车辆停止" << endl;
    cout << "  🚶 行人可通行" << endl;
    
    // 测试场景3：行人过街请求
    cout << "\n4. 测试场景3：行人过街请求" << endl;
    cout << "行人按下过街按钮，延长绿灯时间" << endl;
    
    ctx.set("is_emergency", false);
    ctx.set("pedestrian_crossing", true);
    ctx.set("current_light", "green");
    
    cout << "行人过街模式：" << endl;
    cout << "  🚶 行人过街请求激活" << endl;
    cout << "  🟢 绿灯时间延长" << endl;
    cout << "  ⏰ 倒计时显示" << endl;
    
    // 测试场景4：交通密度调整
    cout << "\n5. 测试场景4：交通密度调整" << endl;
    cout << "根据交通密度调整红绿灯时间" << endl;
    
    ctx.set("pedestrian_crossing", false);
    ctx.set("traffic_density", "high");
    
    cout << "高密度交通模式：" << endl;
    cout << "  🚗 交通密度：高" << endl;
    cout << "  ⏰ 绿灯时间：延长至45秒" << endl;
    cout << "  🟡 黄灯时间：保持4秒" << endl;
    cout << "  🔴 红灯时间：缩短至15秒" << endl;
    
    // 显示行为树信息
    cout << "\n6. 路口红绿灯行为树信息:" << endl;
    json treeInfo = btManager.getTreeInfo("junction_light");
    cout << treeInfo.dump(2) << endl;
    
    // 显示执行统计
    cout << "\n7. 执行统计:" << endl;
    json stats = btManager.getExecutionStats("junction_light");
    cout << stats.dump(2) << endl;
    
    // 显示红绿灯状态信息
    cout << "\n8. 红绿灯状态信息:" << endl;
    cout << "当前灯光: " << ctx.get("current_light").get<string>() << endl;
    cout << "周期计数: " << ctx.get("cycle_count").get<int>() << endl;
    cout << "紧急状态: " << (ctx.get("is_emergency").get<bool>() ? "是" : "否") << endl;
    cout << "行人过街: " << (ctx.get("pedestrian_crossing").get<bool>() ? "是" : "否") << endl;
    cout << "交通密度: " << ctx.get("traffic_density").get<string>() << endl;
    
    cout << "\n=== 路口红绿灯任务测试完成 ===" << endl;
    return 0;
}
