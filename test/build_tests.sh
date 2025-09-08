#!/bin/bash

# Snipper 测试程序构建脚本

set -e

# 获取项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/test"

echo "=== Snipper 测试程序构建 ==="
echo "项目根目录: $PROJECT_ROOT"
echo "构建目录: $BUILD_DIR"
echo "测试目录: $TEST_DIR"

# 检查构建目录是否存在
if [ ! -d "$BUILD_DIR" ]; then
    echo "错误: 构建目录不存在，请先运行 cmake 和 make"
    exit 1
fi

# 设置编译参数
CXX_FLAGS="-std=c++17 -Wall -Wextra"
INCLUDE_FLAGS="-I$PROJECT_ROOT -I$BUILD_DIR/_deps/nlohmann_json-src/include"
LINK_FLAGS="-pthread"

# 创建测试输出目录
mkdir -p "$TEST_DIR/bin"

echo ""
echo "编译测试程序..."

# 编译简单功能测试
echo "  编译 test_simple..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_simple.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_simple"

# 编译优先级演示程序
echo "  编译 test_priority_demo..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_priority_demo.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_priority_demo"

# 编译表达式增强测试
echo "  编译 test_expression_simple..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_expression_simple.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_expression_simple"

# 编译行为树测试
echo "  编译 test_behavior_tree..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_behavior_tree.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_behavior_tree"

# 编译清洁机器人测试
echo "  编译 test_clean_robot..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_clean_robot.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_clean_robot"

# 编译多条件测试
echo "  编译 test_multi_condition..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_multi_condition.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_multi_condition"

# 编译路口红绿灯测试
echo "  编译 test_junction_light..."
g++ $CXX_FLAGS $INCLUDE_FLAGS \
    "$TEST_DIR/test_junction_light.cpp" \
    "$PROJECT_ROOT/runtime/runtime.cpp" \
    "$PROJECT_ROOT/runtime/core/context.cpp" \
    "$PROJECT_ROOT/runtime/core/rule.cpp" \
    "$PROJECT_ROOT/runtime/core/engine.cpp" \
    "$PROJECT_ROOT/runtime/condition/condition_evaluator.cpp" \
    "$PROJECT_ROOT/runtime/condition/operators.cpp" \
    "$PROJECT_ROOT/runtime/expression/expression.cpp" \
    "$PROJECT_ROOT/runtime/priority/priority_manager.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_node.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_parser.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_executor.cpp" \
    "$PROJECT_ROOT/runtime/behavior_tree/bt_manager.cpp" \
    $LINK_FLAGS \
    -o "$TEST_DIR/bin/test_junction_light"

echo ""
echo "构建完成！"
echo ""
echo "运行测试:"
echo "  ./test/bin/test_simple"
echo "  ./test/bin/test_priority_demo"
echo "  ./test/bin/test_expression_simple"
echo "  ./test/bin/test_behavior_tree"
echo "  ./test/bin/test_clean_robot"
echo "  ./test/bin/test_multi_condition"
echo "  ./test/bin/test_junction_light"
echo ""
echo "清理测试文件:"
echo "  rm -rf test/bin"
