#include "../runtime/runtime.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace std;

int main() {
    cout << "=== 条件表达式增强功能测试 ===" << endl;
    
    Engine engine;
    
    // 注册测试动作
    engine.register_action("test_action", [](const json& params, Context& ctx) {
        string message = params.value("message", "No message");
        cout << "  ✓ 执行动作: " << message << endl;
    });
    
    // 创建表达式测试配置
    json config = json::parse(R"({
        "rules": [
            {
                "id": "math_expression_test",
                "when": {
                    "expression": {
                        "op": ">",
                        "left": {
                            "op": "+",
                            "left": "temp",
                            "right": 10
                        },
                        "right": 50
                    }
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "数学表达式测试: (temp + 10) > 50"
                        }
                    }
                ],
                "priority": 100
            },
            {
                "id": "logic_expression_test",
                "when": {
                    "expression": {
                        "op": "&&",
                        "left": {
                            "op": ">",
                            "left": "temp",
                            "right": 30
                        },
                        "right": {
                            "op": "==",
                            "left": "door",
                            "right": "open"
                        }
                    }
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "逻辑表达式测试: temp > 30 && door == 'open'"
                        }
                    }
                ],
                "priority": 200
            },
            {
                "id": "string_expression_test",
                "when": {
                    "expression": {
                        "func": "contains",
                        "args": [
                            "status",
                            "error"
                        ]
                    }
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "字符串表达式测试: contains(status, 'error')"
                        }
                    }
                ],
                "priority": 300
            },
            {
                "id": "complex_expression_test",
                "when": {
                    "expression": {
                        "op": "||",
                        "left": {
                            "op": ">",
                            "left": {
                                "op": "*",
                                "left": "temp",
                                "right": 2
                            },
                            "right": 80
                        },
                        "right": {
                            "op": "&&",
                            "left": {
                                "op": ">=",
                                "left": "pressure",
                                "right": 100
                            },
                            "right": {
                                "func": "starts_with",
                                "args": [
                                    "alarm",
                                    "critical"
                                ]
                            }
                        }
                    }
                },
                "do": [
                    {
                        "action": "test_action",
                        "params": {
                            "message": "复杂表达式测试: (temp * 2 > 80) || (pressure >= 100 && starts_with(alarm, 'critical'))"
                        }
                    }
                ],
                "priority": 400
            }
        ]
    })");
    
    // 加载配置
    engine.load(config);
    
    cout << "\n1. 测试数学表达式:" << endl;
    Context ctx;
    ctx.set("temp", 45);  // (45 + 10) > 50 = true
    engine.tick(ctx);
    
    cout << "\n2. 测试逻辑表达式:" << endl;
    ctx.set("temp", 35);  // 35 > 30 = true
    ctx.set("door", "open");  // door == "open" = true
    engine.tick(ctx);
    
    cout << "\n3. 测试字符串表达式:" << endl;
    ctx.set("status", "system error occurred");  // contains("system error occurred", "error") = true
    engine.tick(ctx);
    
    cout << "\n4. 测试复杂表达式 (条件1满足):" << endl;
    ctx.set("temp", 50);  // (50 * 2) > 80 = true
    ctx.set("pressure", 50);  // 50 >= 100 = false
    ctx.set("alarm", "warning");  // starts_with("warning", "critical") = false
    engine.tick(ctx);
    
    cout << "\n5. 测试复杂表达式 (条件2满足):" << endl;
    ctx.set("temp", 30);  // (30 * 2) > 80 = false
    ctx.set("pressure", 120);  // 120 >= 100 = true
    ctx.set("alarm", "critical system failure");  // starts_with("critical system failure", "critical") = true
    engine.tick(ctx);
    
    cout << "\n6. 测试复杂表达式 (都不满足):" << endl;
    ctx.set("temp", 30);  // (30 * 2) > 80 = false
    ctx.set("pressure", 50);  // 50 >= 100 = false
    ctx.set("alarm", "warning");  // starts_with("warning", "critical") = false
    engine.tick(ctx);
    
    cout << "\n=== 所有表达式测试完成 ===" << endl;
    return 0;
}
