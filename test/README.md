# 测试文件夹

这个文件夹包含snipper项目的各种测试程序和测试配置文件。

## 文件说明

### 测试配置文件
- `task_priority_test.json` - 规则优先级系统测试配置
- `task_basic_test.json` - 基础功能测试配置

### 测试程序
- `test_priority_demo.cpp` - 优先级系统演示程序
- `test_basic_functionality.cpp` - 基础功能测试程序

## 编译和运行测试

### 编译测试程序
```bash
# 在项目根目录下
g++ -std=c++17 -I. -Ibuild/_deps/nlohmann_json-src/include \
    test/test_priority_demo.cpp runtime/runtime.cpp \
    -o test/test_priority_demo
```

### 运行测试
```bash
# 运行优先级演示
./test/test_priority_demo

# 运行基础功能测试
./test/test_basic_functionality
```

## 测试覆盖范围

- ✅ 规则优先级系统
- ✅ 规则组管理
- ✅ 条件评估
- ✅ 动作执行
- ✅ 多线程架构
- [ ] 错误处理
- [ ] 性能测试
- [ ] 边界条件测试

## 添加新测试

1. 在test文件夹中创建新的测试文件
2. 更新此README文档
3. 确保测试程序可以独立编译和运行
4. 测试完成后清理临时文件
