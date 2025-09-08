# snipper
A trigger activates the execution implement

## 项目简介

snipper是一个基于规则引擎的自动化控制系统，可以解析task.json配置文件来执行各种动作。

- **snipper**: 主程序，基于多线程架构的规则引擎运行时
- **groot**: 图形界面编辑器，用于编辑task.json配置文件
- **runtime**: 核心运行时引擎，支持条件评估和动作执行

## 当前功能

- ✅ 规则引擎核心功能
- ✅ 多线程架构（runtime、update、execute线程）
- ✅ JSON配置解析
- ✅ 条件评估（支持复合条件：all、any）
- ✅ 动作注册和执行
- ✅ 规则节流控制
- ✅ 多种比较操作符（==、!=、>、<、>=、<=）

## 功能扩展计划

### 1. 条件表达式增强
- [ ] 数学运算：+, -, *, /, %
- [ ] 逻辑运算：&&, ||, !
- [ ] 字符串操作：contains, starts_with, ends_with
- [ ] 时间条件：time_between, day_of_week
- [ ] 历史数据：avg_last_n, max_last_n, trend

### 2. 规则优先级和依赖管理
- [ ] 规则优先级系统
- [ ] 规则依赖关系（A规则依赖B规则完成）
- [ ] 规则组和命名空间
- [ ] 规则启用/禁用控制

### 3. 数据持久化和状态管理
- [ ] 规则执行历史记录
- [ ] 传感器数据历史存储
- [ ] 规则状态持久化
- [ ] 配置热重载

### 4. 错误处理和监控
- [ ] 规则执行失败重试机制
- [ ] 动作执行超时控制
- [ ] 详细的日志记录系统
- [ ] 健康检查和监控指标

### 5. 高级调度功能
- [ ] 定时规则（cron表达式）
- [ ] 延迟执行和定时器
- [ ] 规则执行频率限制
- [ ] 资源使用监控

### 6. 网络和外部集成
- [ ] HTTP API接口
- [ ] WebSocket实时通信
- [ ] MQTT消息订阅
- [ ] 数据库连接器
- [ ] 外部服务调用

### 7. 安全和权限管理
- [ ] 动作执行权限控制
- [ ] 规则修改权限验证
- [ ] 敏感数据加密
- [ ] 审计日志

### 8. 性能优化
- [ ] 规则执行缓存
- [ ] 条件预编译
- [ ] 批量数据处理
- [ ] 内存使用优化


## 安装依赖


## 构建和运行

```bash
# 构建项目
mkdir build && cd build
cmake ..
make

# 运行主程序
cp ../task.json .
./bin/snipper

# 运行图形编辑器
./bin/groot
```

## 测试

项目包含完整的测试套件，位于 `test/` 目录：

```bash
# 构建测试程序
./test/build_tests.sh

# 运行所有测试
./test/run_tests.sh

# 清理测试文件
rm -rf test/bin
```

### 测试覆盖范围
- ✅ 基础功能测试
- ✅ 规则优先级系统
- ✅ 规则组管理
- ✅ 条件评估
- ✅ 动作执行
- ✅ 多线程架构

## 配置文件示例

```json
{
    "rules": [
        {
            "id": "r1",
            "when": {
                "all": [
                    {"left": "temp", "op": ">", "right": 40},
                    {"left": "door", "op": "==", "right": "open"}
                ]
            },
            "do": [
                {"action": "fan_on", "params": {"level": 3}},
                {"action": "notify", "params": {"text": "High temp with door open"}}
            ],
            "mode": "repeat",
            "throttle_ms": 1000
        }
    ]
}
```

## 开发状态

- [x] 基础规则引擎
- [x] 多线程架构
- [x] JSON配置解析
- [x] 条件评估系统
- [x] 动作执行框架
- [x] 规则优先级系统
- [ ] 图形界面编辑器（groot）
- [ ] 高级条件表达式
- [ ] 数据持久化
- [ ] 网络API接口

