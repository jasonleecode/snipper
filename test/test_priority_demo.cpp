#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 规则优先级系统演示 ===" << endl;
    
    Engine engine;
    
    // 注册动作
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
    
    // 加载配置
    ifstream file("task_priority_test.json");
    json config = json::parse(file);
    engine.load(config);
    
    cout << "\n1. 所有规则（按优先级排序）:" << endl;
    auto rules = engine.get_all_rules();
    for (const auto& rule : rules) {
        cout << "   " << rule.id << " (优先级: " << rule.priority 
             << ", 组: " << (rule.group.empty() ? "无" : rule.group) << ")" << endl;
    }
    
    // 显示规则组
    cout << "\n2. 规则组状态:" << endl;
    cout << "   safety: " << (engine.get_rules_by_group("safety").size() > 0 ? "启用" : "禁用") << endl;
    cout << "   temperature: " << (engine.get_rules_by_group("temperature").size() > 0 ? "启用" : "禁用") << endl;
    cout << "   reminders: " << (engine.get_rules_by_group("reminders").size() > 0 ? "启用" : "禁用") << endl;
    
    // 测试规则组禁用
    cout << "\n3. 禁用temperature组..." << endl;
    engine.disable_rule_group("temperature");
    
    // 创建测试上下文
    Context ctx;
    ctx.set("temp", 45);
    ctx.set("door", "open");
    ctx.set("emergency_button", "not_pressed");
    
    cout << "\n4. 执行规则（temperature组已禁用）:" << endl;
    engine.tick(ctx);
    
    cout << "\n5. 重新启用temperature组..." << endl;
    engine.enable_rule_group("temperature");
    
    cout << "\n6. 再次执行规则:" << endl;
    engine.tick(ctx);
    
    cout << "\n7. 测试紧急按钮（最高优先级）:" << endl;
    ctx.set("emergency_button", "pressed");
    engine.tick(ctx);
    
    return 0;
}