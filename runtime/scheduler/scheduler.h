#pragma once

#include "cron_parser.h"
#include "timer.h"
#include "frequency_limiter.h"
#include "resource_monitor.h"
#include <memory>
#include <string>
#include <functional>

namespace snipper {
namespace scheduler {

/**
 * 调度任务类型
 */
enum class TaskType {
    RULE_EXECUTION,    // 规则执行
    BEHAVIOR_TREE,     // 行为树执行
    CUSTOM_ACTION      // 自定义动作
};

/**
 * 调度任务配置
 */
struct TaskConfig {
    std::string id;
    TaskType type;
    std::string targetId;              // 目标ID（规则ID、行为树ID等）
    std::string cronExpression;        // Cron表达式
    std::chrono::milliseconds interval; // 执行间隔
    int repeatCount = -1;              // 重复次数，-1表示无限
    bool enabled = true;               // 是否启用
    
    // 频率限制配置
    FrequencyLimiter::LimitConfig frequencyLimit;
    
    // 资源限制配置
    ResourceLimit resourceLimit;
    
    TaskConfig() = default;
};

/**
 * 调度任务状态
 */
enum class TaskStatus {
    PENDING,    // 等待执行
    RUNNING,    // 正在执行
    COMPLETED,  // 已完成
    FAILED,     // 执行失败
    CANCELLED,  // 已取消
    DISABLED    // 已禁用
};

/**
 * 调度任务信息
 */
struct TaskInfo {
    TaskConfig config;
    TaskStatus status;
    std::chrono::system_clock::time_point nextExecution;
    std::chrono::system_clock::time_point lastExecution;
    int executionCount = 0;
    int successCount = 0;
    int failureCount = 0;
    std::string lastError;
    
    TaskInfo() : status(TaskStatus::PENDING) {}
};

/**
 * 调度器回调函数类型
 */
using TaskCallback = std::function<bool(const std::string& taskId, const std::string& targetId)>;

/**
 * 高级调度器
 * 集成定时器、频率限制和资源监控功能
 */
class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    /**
     * 启动调度器
     */
    void start();

    /**
     * 停止调度器
     */
    void stop();

    /**
     * 注册任务回调函数
     * @param taskType 任务类型
     * @param callback 回调函数
     */
    void registerCallback(TaskType taskType, TaskCallback callback);

    /**
     * 创建定时任务
     * @param config 任务配置
     * @return 是否创建成功
     */
    bool createTask(const TaskConfig& config);

    /**
     * 创建Cron任务
     * @param id 任务ID
     * @param targetId 目标ID
     * @param cronExpression Cron表达式
     * @param taskType 任务类型
     * @return 是否创建成功
     */
    bool createCronTask(const std::string& id,
                       const std::string& targetId,
                       const std::string& cronExpression,
                       TaskType taskType = TaskType::RULE_EXECUTION);

    /**
     * 创建重复任务
     * @param id 任务ID
     * @param targetId 目标ID
     * @param interval 执行间隔
     * @param repeatCount 重复次数
     * @param taskType 任务类型
     * @return 是否创建成功
     */
    bool createRepeatTask(const std::string& id,
                         const std::string& targetId,
                         std::chrono::milliseconds interval,
                         int repeatCount = -1,
                         TaskType taskType = TaskType::RULE_EXECUTION);

    /**
     * 取消任务
     * @param taskId 任务ID
     * @return 是否取消成功
     */
    bool cancelTask(const std::string& taskId);

    /**
     * 启用/禁用任务
     * @param taskId 任务ID
     * @param enabled 是否启用
     * @return 是否操作成功
     */
    bool setTaskEnabled(const std::string& taskId, bool enabled);

    /**
     * 获取任务信息
     * @param taskId 任务ID
     * @return 任务信息，如果不存在则返回nullptr
     */
    std::shared_ptr<TaskInfo> getTaskInfo(const std::string& taskId);

    /**
     * 获取所有任务信息
     * @return 所有任务信息列表
     */
    std::vector<std::shared_ptr<TaskInfo>> getAllTasks();

    /**
     * 获取调度器统计信息
     */
    struct SchedulerStats {
        int totalTasks = 0;
        int activeTasks = 0;
        int completedTasks = 0;
        int failedTasks = 0;
        int cancelledTasks = 0;
        int disabledTasks = 0;
        int totalExecutions = 0;
        int successfulExecutions = 0;
        int failedExecutions = 0;
        double successRate = 0.0;
        
        // 资源统计
        ResourceMonitor::GlobalStats resourceStats;
        
        // 频率限制统计
        FrequencyLimiter::Stats frequencyStats;
    };

    SchedulerStats getStats();

    /**
     * 清理过期的任务和统计数据
     * @param maxAgeHours 最大保留时间（小时）
     */
    void cleanup(int maxAgeHours = 24);

private:
    std::unique_ptr<TimerManager> timerManager_;
    std::unique_ptr<FrequencyLimiter> frequencyLimiter_;
    std::unique_ptr<ResourceMonitor> resourceMonitor_;
    
    std::map<TaskType, TaskCallback> callbacks_;
    std::map<std::string, std::shared_ptr<TaskInfo>> tasks_;
    std::mutex tasksMutex_;
    
    std::atomic<bool> running_;

    /**
     * 执行任务
     * @param taskId 任务ID
     */
    void executeTask(const std::string& taskId);

    /**
     * 创建任务包装器
     * @param taskId 任务ID
     * @return 定时器回调函数
     */
    TimerCallback createTaskWrapper(const std::string& taskId);

    /**
     * 更新任务状态
     * @param taskId 任务ID
     * @param status 新状态
     */
    void updateTaskStatus(const std::string& taskId, TaskStatus status);

    /**
     * 记录任务执行结果
     * @param taskId 任务ID
     * @param success 是否成功
     * @param executionTimeMs 执行时间
     * @param error 错误信息
     */
    void recordTaskExecution(const std::string& taskId, 
                           bool success, 
                           size_t executionTimeMs = 0,
                           const std::string& error = "");
};

} // namespace scheduler
} // namespace snipper