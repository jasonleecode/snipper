#include "scheduler.h"
#include <algorithm>

namespace snipper {
namespace scheduler {

Scheduler::Scheduler() 
    : timerManager_(std::make_unique<TimerManager>())
    , frequencyLimiter_(std::make_unique<FrequencyLimiter>())
    , resourceMonitor_(std::make_unique<ResourceMonitor>())
    , running_(false) {
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    timerManager_->start();
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    timerManager_->stop();
}

void Scheduler::registerCallback(TaskType taskType, TaskCallback callback) {
    callbacks_[taskType] = callback;
}

bool Scheduler::createTask(const TaskConfig& config) {
    if (config.id.empty() || config.targetId.empty()) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        if (tasks_.find(config.id) != tasks_.end()) {
            return false; // 任务已存在
        }
    }
    
    // 创建任务信息
    auto taskInfo = std::make_shared<TaskInfo>();
    taskInfo->config = config;
    taskInfo->status = config.enabled ? TaskStatus::PENDING : TaskStatus::DISABLED;
    
    // 设置频率限制
    frequencyLimiter_->setLimit(config.id, config.frequencyLimit);
    
    // 设置资源监控
    resourceMonitor_->startMonitoring(config.id, config.resourceLimit);
    
    // 创建定时器
    bool timerCreated = false;
    if (!config.cronExpression.empty()) {
        timerCreated = timerManager_->createCronTimer(
            config.id, config.cronExpression, createTaskWrapper(config.id));
    } else if (config.interval.count() > 0) {
        timerCreated = timerManager_->createRepeatTimer(
            config.id, config.interval, createTaskWrapper(config.id), config.repeatCount);
    }
    
    if (timerCreated) {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        tasks_[config.id] = taskInfo;
        return true;
    }
    
    return false;
}

bool Scheduler::createCronTask(const std::string& id,
                              const std::string& targetId,
                              const std::string& cronExpression,
                              TaskType taskType) {
    TaskConfig config;
    config.id = id;
    config.targetId = targetId;
    config.cronExpression = cronExpression;
    config.type = taskType;
    
    return createTask(config);
}

bool Scheduler::createRepeatTask(const std::string& id,
                                const std::string& targetId,
                                std::chrono::milliseconds interval,
                                int repeatCount,
                                TaskType taskType) {
    TaskConfig config;
    config.id = id;
    config.targetId = targetId;
    config.interval = interval;
    config.repeatCount = repeatCount;
    config.type = taskType;
    
    return createTask(config);
}

bool Scheduler::cancelTask(const std::string& taskId) {
    bool timerCancelled = timerManager_->cancelTimer(taskId);
    
    if (timerCancelled) {
        updateTaskStatus(taskId, TaskStatus::CANCELLED);
        return true;
    }
    
    return false;
}

bool Scheduler::setTaskEnabled(const std::string& taskId, bool enabled) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    auto& taskInfo = it->second;
    taskInfo->config.enabled = enabled;
    taskInfo->status = enabled ? TaskStatus::PENDING : TaskStatus::DISABLED;
    
    return true;
}

std::shared_ptr<TaskInfo> Scheduler::getTaskInfo(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<TaskInfo>> Scheduler::getAllTasks() {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    std::vector<std::shared_ptr<TaskInfo>> result;
    for (const auto& pair : tasks_) {
        result.push_back(pair.second);
    }
    
    return result;
}

Scheduler::SchedulerStats Scheduler::getStats() {
    SchedulerStats stats;
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        stats.totalTasks = tasks_.size();
        
        for (const auto& pair : tasks_) {
            const auto& taskInfo = pair.second;
            
            switch (taskInfo->status) {
                case TaskStatus::PENDING:
                case TaskStatus::RUNNING:
                    stats.activeTasks++;
                    break;
                case TaskStatus::COMPLETED:
                    stats.completedTasks++;
                    break;
                case TaskStatus::FAILED:
                    stats.failedTasks++;
                    break;
                case TaskStatus::CANCELLED:
                    stats.cancelledTasks++;
                    break;
                case TaskStatus::DISABLED:
                    stats.disabledTasks++;
                    break;
            }
            
            stats.totalExecutions += taskInfo->executionCount;
            stats.successfulExecutions += taskInfo->successCount;
            stats.failedExecutions += taskInfo->failureCount;
        }
    }
    
    if (stats.totalExecutions > 0) {
        stats.successRate = static_cast<double>(stats.successfulExecutions) / stats.totalExecutions;
    }
    
    stats.resourceStats = resourceMonitor_->getGlobalStats();
    stats.frequencyStats = frequencyLimiter_->getStats();
    
    return stats;
}

void Scheduler::cleanup(int maxAgeHours) {
    resourceMonitor_->cleanupExpiredData(maxAgeHours);
    frequencyLimiter_->cleanup();
}

void Scheduler::executeTask(const std::string& taskId) {
    auto taskInfo = getTaskInfo(taskId);
    if (!taskInfo || !taskInfo->config.enabled) {
        return;
    }
    
    // 检查频率限制
    if (!frequencyLimiter_->tryAcquire(taskId)) {
        return; // 频率限制
    }
    
    // 检查资源限制
    if (!resourceMonitor_->checkLimits(taskId)) {
        updateTaskStatus(taskId, TaskStatus::FAILED);
        recordTaskExecution(taskId, false, 0, "Resource limit exceeded");
        return;
    }
    
    updateTaskStatus(taskId, TaskStatus::RUNNING);
    taskInfo->lastExecution = std::chrono::system_clock::now();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    bool success = false;
    std::string error;
    
    try {
        // 查找并执行回调函数
        auto callbackIt = callbacks_.find(taskInfo->config.type);
        if (callbackIt != callbacks_.end()) {
            success = callbackIt->second(taskId, taskInfo->config.targetId);
        } else {
            error = "No callback registered for task type";
        }
    } catch (const std::exception& e) {
        error = e.what();
    } catch (...) {
        error = "Unknown error occurred";
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 记录执行结果
    recordTaskExecution(taskId, success, executionTime.count(), error);
    
    // 更新任务状态
    if (success) {
        if (taskInfo->config.repeatCount == -1 || 
            taskInfo->executionCount < taskInfo->config.repeatCount) {
            updateTaskStatus(taskId, TaskStatus::PENDING);
        } else {
            updateTaskStatus(taskId, TaskStatus::COMPLETED);
        }
    } else {
        updateTaskStatus(taskId, TaskStatus::FAILED);
    }
}

TimerCallback Scheduler::createTaskWrapper(const std::string& taskId) {
    return [this, taskId]() {
        executeTask(taskId);
    };
}

void Scheduler::updateTaskStatus(const std::string& taskId, TaskStatus status) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second->status = status;
    }
}

void Scheduler::recordTaskExecution(const std::string& taskId, 
                                  bool success, 
                                  size_t executionTimeMs,
                                  const std::string& error) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        auto& taskInfo = it->second;
        taskInfo->executionCount++;
        
        if (success) {
            taskInfo->successCount++;
        } else {
            taskInfo->failureCount++;
            taskInfo->lastError = error;
        }
    }
    
    // 记录资源使用
    resourceMonitor_->recordUsage(taskId, 0, executionTimeMs, success);
}

} // namespace scheduler
} // namespace snipper
