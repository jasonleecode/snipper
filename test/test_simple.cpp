#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== Snipper 简单功能测试 ===" << endl;
    
    Engine engine;
    
    // 注册测试动作
    engine.register_action("test_action", [](const json& params, Context& ctx) {
        string message = params.value("message", "No message");
        cout << "  ✓ 执行动作: " << message << endl;
    });
    
    // 创建简单的测试配置
    json config = json::parse(R"({
        "rules": [
            {
                "id": "test_rule_1",
                "when": {
                    "left": "test_var",
                    "op": "==",
                    "right": "value1"
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "规则1触发"
                        }
                    }
                ],
                "mode": "once",
                "priority": 100
            },
            {
                "id": "test_rule_2",
                "when": {
                    "left": "test_var",
                    "op": "==",
                    "right": "value2"
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "规则2触发"
                        }
                    }
                ],
                "mode": "repeat",
                "priority": 200
            }
        ]
    })");
    
    // 加载配置
    engine.load(config);
    
    cout << "\n1. 测试规则加载:" << endl;
    auto rules = engine.get_all_rules();
    cout << "   加载了 " << rules.size() << " 个规则" << endl;
    for (const auto& rule : rules) {
        cout << "   - " << rule.id << " (优先级: " << rule.priority << ")" << endl;
    }
    
    cout << "\n2. 测试条件评估:" << endl;
    Context ctx;
    
    // 测试规则1
    cout << "   测试规则1 (test_var == value1):" << endl;
    ctx.set("test_var", "value1");
    engine.tick(ctx);
    
    // 测试规则2
    cout << "   测试规则2 (test_var == value2):" << endl;
    ctx.set("test_var", "value2");
    engine.tick(ctx);
    
    // 测试不满足条件
    cout << "   测试不满足条件 (test_var == value3):" << endl;
    ctx.set("test_var", "value3");
    engine.tick(ctx);
    
    cout << "\n3. 测试规则优先级:" << endl;
    // 创建优先级测试配置
    json priorityConfig = json::parse(R"({
        "rules": [
            {
                "id": "low_priority",
                "when": {
                    "left": "test",
                    "op": "==",
                    "right": "true"
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "低优先级规则"
                        }
                    }
                ],
                "priority": 300
            },
            {
                "id": "high_priority",
                "when": {
                    "left": "test",
                    "op": "==",
                    "right": "true"
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "高优先级规则"
                        }
                    }
                ],
                "priority": 100
            }
        ]
    })");
    
    engine.load(priorityConfig);
    ctx.set("test", "true");
    cout << "   执行规则（应该按优先级顺序）:" << endl;
    engine.tick(ctx);
    
    cout << "\n=== 所有测试完成 ===" << endl;
    return 0;
}
