#include "runtime/runtime.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace nlohmann;
using namespace std;

// 更新任务, 通过网络接收新的task.json
void UpdateTask();

// 初始化任务
void InitTask();

// 执行任务 
void ExecuteTask();

int main() {
    try {
        Engine engine;
        
        // 注册一些示例动作
        engine.register_action("fan_on", [](const json& params, Context& ctx) {
            int level = params.value("level", 1);
            cout << "Fan turned on at level " << level << endl;
        });
        
        engine.register_action("notify", [](const json& params, Context& ctx) {
            string text = params.value("text", "");
            cout << "Notification: " << text << endl;
        });
        
        engine.register_action("emergency_stop", [](const json& params, Context& ctx) {
            string reason = params.value("reason", "");
            cout << "🚨 EMERGENCY STOP: " << reason << endl;
        });
        
        engine.register_action("alarm", [](const json& params, Context& ctx) {
            string level = params.value("level", "medium");
            cout << "🚨 ALARM (" << level << " level): Critical condition detected!" << endl;
        });
        
        engine.register_action("heater_on", [](const json& params, Context& ctx) {
            int level = params.value("level", 1);
            cout << "Heater turned on at level " << level << endl;
        });
        
    // 加载配置文件
    ifstream file("task.json");
        if (!file.is_open()) {
            cerr << "Error: Cannot open task.json" << endl;
            return 1;
        }
        
        json config = json::parse(file);
        engine.load(config);
        
        cout << "Snipper started successfully!" << endl;
        cout << "Rules loaded with priority system:" << endl;
        
        // 显示规则优先级信息
        auto rules = engine.get_all_rules();
        for (const auto& rule : rules) {
            cout << "  Rule " << rule.id << " (priority: " << rule.priority 
                 << ", group: " << (rule.group.empty() ? "none" : rule.group) << ")" << endl;
        }
        cout << endl;
        
        cout << "Press Ctrl+C to exit..." << endl;

        // 创建Context用于存储传感器数据
        Context ctx;
        
        // 模拟一些传感器数据
        ctx.set("temp", 45);  // 温度45度
        ctx.set("door", "open");  // 门是开着的
        ctx.set("emergency_button", "not_pressed");  // 紧急按钮未按下

        // 创建runtime线程
        thread runtimeThread([&]() {
            while (true) {
                engine.onSensorUpdate();
                engine.tick(ctx);
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        });

        // 创建更新任务线程
        thread updateTaskThread([&]() {
            while (true) {
                UpdateTask();
            }
        });

        // 创建执行任务线程
        thread executeTaskThread([&]() {
            while (true) {
                ExecuteTask();
            }
        });

        // 主线程等待
        runtimeThread.join();
        updateTaskThread.join();
        executeTaskThread.join();
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}