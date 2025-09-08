#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "runtime/scheduler/scheduler.h"

using namespace snipper::scheduler;

// 模拟规则执行回调
bool ruleExecutionCallback(const std::string& taskId, const std::string& targetId) {
    std::cout << "执行规则: " << targetId << " (任务ID: " << taskId << ")" << std::endl;
    return true;
}

// 模拟行为树执行回调
bool behaviorTreeCallback(const std::string& taskId, const std::string& targetId) {
    std::cout << "执行行为树: " << targetId << " (任务ID: " << taskId << ")" << std::endl;
    return true;
}

// 模拟自定义动作回调
bool customActionCallback(const std::string& taskId, const std::string& targetId) {
    std::cout << "执行自定义动作: " << targetId << " (任务ID: " << taskId << ")" << std::endl;
    return true;
}

void testCronParser() {
    std::cout << "\n=== 测试Cron表达式解析器 ===" << std::endl;
    
    // 测试有效的cron表达式
    std::vector<std::string> validExpressions = {
        "0 9 * * 1-5",    // 每个工作日上午9点
        "*/5 * * * *",    // 每5分钟
        "0 0 1 * *",      // 每月1号午夜
        "30 14 * * 0",    // 每周日下午2:30
        "0 0 * * 0"       // 每周日午夜
    };
    
    for (const auto& expr : validExpressions) {
        bool valid = CronParser::isValid(expr);
        std::cout << "表达式 '" << expr << "' 是否有效: " << (valid ? "是" : "否") << std::endl;
        
        if (valid) {
            auto cron = CronParser::parse(expr);
            auto now = std::chrono::system_clock::now();
            auto nextMatch = CronParser::nextMatch(cron, now);
            
            auto time_t = std::chrono::system_clock::to_time_t(nextMatch);
            std::cout << "  下次匹配时间: " << std::ctime(&time_t);
        }
    }
}

void testTimerManager() {
    std::cout << "\n=== 测试定时器管理器 ===" << std::endl;
    
    TimerManager timerManager;
    timerManager.start();
    
    // 创建一次性定时器
    timerManager.createOnceTimer("once_timer", std::chrono::seconds(2), []() {
        std::cout << "一次性定时器执行!" << std::endl;
    });
    
    // 创建重复定时器
    timerManager.createRepeatTimer("repeat_timer", std::chrono::seconds(3), []() {
        std::cout << "重复定时器执行!" << std::endl;
    }, 3);
    
    // 创建Cron定时器
    timerManager.createCronTimer("cron_timer", "*/10 * * * * *", []() {
        std::cout << "Cron定时器执行!" << std::endl;
    });
    
    // 等待一段时间观察执行
    std::this_thread::sleep_for(std::chrono::seconds(15));
    
    // 获取统计信息
    auto stats = timerManager.getStats();
    std::cout << "定时器统计: 总数=" << stats.totalTimers 
              << ", 活跃=" << stats.activeTimers 
              << ", 完成=" << stats.completedTimers << std::endl;
    
    timerManager.stop();
}

void testFrequencyLimiter() {
    std::cout << "\n=== 测试频率限制器 ===" << std::endl;
    
    FrequencyLimiter limiter;
    
    // 设置限制：每分钟最多5次请求
    FrequencyLimiter::LimitConfig config;
    config.maxRequests = 5;
    config.windowMs = std::chrono::minutes(1);
    config.strategy = FrequencyLimiter::Strategy::SLIDING_WINDOW;
    
    limiter.setLimit("test_rule", config);
    
    // 模拟请求
    for (int i = 0; i < 10; ++i) {
        auto result = limiter.checkLimit("test_rule");
        std::cout << "请求 " << (i + 1) << ": " 
                  << (result.allowed ? "允许" : "拒绝")
                  << ", 剩余: " << result.remainingRequests << std::endl;
        
        if (result.allowed) {
            // 模拟请求处理
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    auto stats = limiter.getStats();
    std::cout << "频率限制统计: 总请求=" << stats.totalRequests 
              << ", 被阻止=" << stats.blockedRequests 
              << ", 阻止率=" << (stats.blockRate * 100) << "%" << std::endl;
}

void testResourceMonitor() {
    std::cout << "\n=== 测试资源监控器 ===" << std::endl;
    
    ResourceMonitor monitor;
    
    // 设置资源限制
    ResourceLimit limits;
    limits.maxMemoryUsage = 1024 * 1024;  // 1MB
    limits.maxCpuTimeMs = 5000;           // 5秒
    limits.maxExecutionCount = 10;        // 最多10次执行
    limits.maxErrorRate = 0.2;            // 最大20%错误率
    
    monitor.startMonitoring("test_task", limits);
    
    // 模拟资源使用
    for (int i = 0; i < 8; ++i) {
        // 模拟内存使用
        size_t memoryDelta = 100 * 1024;  // 100KB
        size_t executionTime = 500;       // 500ms
        bool success = (i < 6);           // 前6次成功，后2次失败
        
        monitor.recordUsage("test_task", memoryDelta, executionTime, success);
        
        auto status = monitor.getResourceStatus("test_task");
        std::cout << "执行 " << (i + 1) << ": "
                  << "内存使用=" << (status.currentUsage.memoryUsage.load() / 1024) << "KB, "
                  << "CPU时间=" << status.currentUsage.cpuTimeMs.load() << "ms, "
                  << "错误率=" << (status.errorRate * 100) << "%, "
                  << "在限制内=" << (status.withinLimits ? "是" : "否") << std::endl;
        
        if (!status.withinLimits) {
            std::cout << "  违反原因: " << status.violationReason << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto globalStats = monitor.getGlobalStats();
    std::cout << "全局资源统计: 总内存=" << (globalStats.totalMemoryUsage / 1024) << "KB, "
              << "总CPU时间=" << globalStats.totalCpuTimeMs << "ms, "
              << "总执行次数=" << globalStats.totalExecutions << std::endl;
}

void testScheduler() {
    std::cout << "\n=== 测试高级调度器 ===" << std::endl;
    
    Scheduler scheduler;
    scheduler.start();
    
    // 注册回调函数
    scheduler.registerCallback(TaskType::RULE_EXECUTION, ruleExecutionCallback);
    scheduler.registerCallback(TaskType::BEHAVIOR_TREE, behaviorTreeCallback);
    scheduler.registerCallback(TaskType::CUSTOM_ACTION, customActionCallback);
    
    // 创建不同类型的任务
    scheduler.createCronTask("daily_rule", "rule_001", "0 9 * * *", TaskType::RULE_EXECUTION);
    scheduler.createRepeatTask("periodic_bt", "bt_001", std::chrono::seconds(5), 3, TaskType::BEHAVIOR_TREE);
    scheduler.createCronTask("hourly_action", "action_001", "0 * * * *", TaskType::CUSTOM_ACTION);
    
    // 等待一段时间观察执行
    std::this_thread::sleep_for(std::chrono::seconds(20));
    
    // 获取任务信息
    auto allTasks = scheduler.getAllTasks();
    std::cout << "\n任务列表:" << std::endl;
    for (const auto& task : allTasks) {
        std::cout << "任务ID: " << task->config.id 
                  << ", 目标: " << task->config.targetId
                  << ", 状态: " << static_cast<int>(task->status)
                  << ", 执行次数: " << task->executionCount << std::endl;
    }
    
    // 获取统计信息
    auto stats = scheduler.getStats();
    std::cout << "\n调度器统计:" << std::endl;
    std::cout << "总任务数: " << stats.totalTasks << std::endl;
    std::cout << "活跃任务: " << stats.activeTasks << std::endl;
    std::cout << "完成任务: " << stats.completedTasks << std::endl;
    std::cout << "失败任务: " << stats.failedTasks << std::endl;
    std::cout << "成功率: " << (stats.successRate * 100) << "%" << std::endl;
    
    scheduler.stop();
}

int main() {
    std::cout << "=== 高级调度功能测试程序 ===" << std::endl;
    
    try {
        testCronParser();
        testTimerManager();
        testFrequencyLimiter();
        testResourceMonitor();
        testScheduler();
        
        std::cout << "\n=== 所有测试完成 ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
