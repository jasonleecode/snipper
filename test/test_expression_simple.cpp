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
    
    // 创建简单的表达式测试配置
    json config = json::parse(R"({
        "rules": [
            {
                "id": "math_test",
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
                            "message": "数学表达式: (temp + 10) > 50"
                        }
                    }
                ],
                "priority": 100
            },
            {
                "id": "logic_test",
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
                            "message": "逻辑表达式: temp > 30 && door == open"
                        }
                    }
                ],
                "priority": 200
            }
        ]
    })");
    
    // 加载配置
    engine.load(config);
    
    cout << "\n加载的规则数量: " << engine.get_all_rules().size() << endl;
    
    cout << "\n1. 测试数学表达式:" << endl;
    Context ctx;
    ctx.set("temp", 45);  // (45 + 10) > 50 = true
    engine.tick(ctx);
    
    cout << "\n2. 测试逻辑表达式:" << endl;
    ctx.set("temp", 35);  // 35 > 30 = true
    ctx.set("door", "open");  // door == "open" = true
    engine.tick(ctx);
    
    cout << "\n3. 测试不满足条件:" << endl;
    ctx.set("temp", 20);  // 20 > 30 = false
    ctx.set("door", "closed");  // door == "open" = false
    engine.tick(ctx);
    
    cout << "\n=== 表达式测试完成 ===" << endl;
    return 0;
}
