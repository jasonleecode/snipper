#!/bin/bash

# Snipper 测试运行脚本

set -e

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$TEST_DIR/bin"

echo "=== Snipper 测试运行 ==="

# 检查测试程序是否存在
if [ ! -d "$BIN_DIR" ]; then
    echo "错误: 测试程序未构建，请先运行 ./test/build_tests.sh"
    exit 1
fi

echo ""
echo "1. 运行简单功能测试..."
echo "----------------------------------------"
if [ -f "$BIN_DIR/test_simple" ]; then
    "$BIN_DIR/test_simple"
    echo "简单功能测试完成 ✓"
else
    echo "错误: test_simple 不存在"
fi

echo ""
echo "2. 运行优先级系统演示..."
echo "----------------------------------------"
if [ -f "$BIN_DIR/test_priority_demo" ]; then
    cd "$BIN_DIR"
    ./test_priority_demo
    echo "优先级系统演示完成 ✓"
    cd "$TEST_DIR"
else
    echo "错误: test_priority_demo 不存在"
fi

echo ""
echo "3. 运行表达式增强测试..."
echo "----------------------------------------"
if [ -f "$BIN_DIR/test_expression_simple" ]; then
    "$BIN_DIR/test_expression_simple"
    echo "表达式增强测试完成 ✓"
else
    echo "错误: test_expression_simple 不存在"
fi

echo ""
echo "=== 所有测试完成 ==="
echo ""
echo "清理测试文件:"
echo "  rm -rf test/bin"
