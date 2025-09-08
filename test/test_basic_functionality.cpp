#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== Snipper 基础功能测试 ===" << endl;
    
    Engine engine;
    
    // 注册测试动作
    engine.register_action("test_action", [](const json& params, Context& ctx) {
        string message = params.value("message", "No message");
        cout << "  ✓ 执行动作: " << message << endl;
    });
    
    engine.register_action("set_value", [](const json& params, Context& ctx) {
        string key = params.value("key", "");
        string value = params.value("value", "");
        ctx.set(key, value);
        cout << "  ✓ 设置值: " << key << " = " << value << endl;
    });
    
    // 创建测试配置
    json testConfig = {
        {"rules", {
            {
                {"id", "test_rule_1"},
                {"when", {
                    {"left", "test_var"},
                    {"op", "=="},
                    {"right", "value1"}
                }},
                {"do", {
                    {{"action", "test_action"}, {"params", {{"message", "规则1触发"}}}}
                }},
                {"mode", "once"},
                {"priority", 100}
            },
            {
                {"id", "test_rule_2"},
                {"when", {
                    {"all", {
                        {{"left", "test_var"}, {"op", "=="}, {"right", "value2"}},
                        {{"left", "counter"}, {"op", ">="}, {"right", 3}}
                    }}
                }},
                {"do", {
                    {{"action", "test_action"}, {"params", {{"message", "规则2触发（复合条件）"}}}},
                    {{"action", "set_value"}, {"params", {{"key", "result"}, {"value", "success"}}}}
                }},
                {"mode", "repeat"},
                {"throttle_ms", 1000},
                {"priority", 200}
            }
        }}
    };
    
    // 加载配置
    engine.load(testConfig);
    
    cout << "\n1. 测试规则加载:" << endl;
    auto rules = engine.get_all_rules();
    cout << "   加载了 " << rules.size() << " 个规则" << endl;
    for (const auto& rule : rules) {
        cout << "   - " << rule.id << " (优先级: " << rule.priority << ")" << endl;
    }
    
    cout << "\n2. 测试条件评估:" << endl;
    Context ctx;
    
    // 测试简单条件
    cout << "   测试简单条件 (test_var == value1):" << endl;
    ctx.set("test_var", "value1");
    engine.tick(ctx);
    
    // 测试复合条件
    cout << "   测试复合条件 (test_var == value2 AND counter >= 3):" << endl;
    ctx.set("test_var", "value2");
    ctx.set("counter", 5);
    engine.tick(ctx);
    
    // 测试不满足条件
    cout << "   测试不满足条件 (test_var == value2 AND counter < 3):" << endl;
    ctx.set("counter", 1);
    engine.tick(ctx);
    
    cout << "\n3. 测试规则优先级:" << endl;
    // 创建优先级测试配置
    json priorityConfig = {
        {"rules", {
            {
                {"id", "low_priority"},
                {"when", {{"left", "test"}, {"op", "=="}, {"right", "true"}}},
                {"do", {{{"action", "test_action"}, {"params", {{"message", "低优先级规则"}}}}},
                {"priority", 300}
            },
            {
                {"id", "high_priority"},
                {"when", {{"left", "test"}, {"op", "=="}, {"right", "true"}}},
                {"do", {{{"action", "test_action"}, {"params", {{"message", "高优先级规则"}}}}},
                {"priority", 100}
            }
        }}
    };
    
    engine.load(priorityConfig);
    ctx.set("test", "true");
    cout << "   执行规则（应该按优先级顺序）:" << endl;
    engine.tick(ctx);
    
    cout << "\n4. 测试规则组管理:" << endl;
    // 创建规则组测试配置
    json groupConfig = {
        {"rules", {
            {
                {"id", "group_rule_1"},
                {"when", {{"left", "test"}, {"op", "=="}, {"right", "true"}}},
                {"do", {{{"action", "test_action"}, {"params", {{"message", "组规则1"}}}}},
                {"group", "test_group"}
            },
            {
                {"id", "group_rule_2"},
                {"when", {{"left", "test"}, {"op", "=="}, {"right", "true"}}},
                {"do", {{{"action", "test_action"}, {"params", {{"message", "组规则2"}}}}},
                {"group", "test_group"}
            }
        }}
    };
    
    engine.load(groupConfig);
    cout << "   启用规则组前:" << endl;
    engine.tick(ctx);
    
    cout << "   禁用规则组:" << endl;
    engine.disable_rule_group("test_group");
    engine.tick(ctx);
    
    cout << "   重新启用规则组:" << endl;
    engine.enable_rule_group("test_group");
    engine.tick(ctx);
    
    cout << "\n5. 测试节流功能:" << endl;
    cout << "   连续执行规则（应该被节流）:" << endl;
    for (int i = 0; i < 3; i++) {
        cout << "   第 " << (i+1) << " 次执行:" << endl;
        engine.tick(ctx);
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    cout << "\n=== 所有测试完成 ===" << endl;
    return 0;
}