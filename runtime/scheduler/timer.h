#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace snipper {
namespace scheduler {

/**
 * Timer callback function type
 */
using TimerCallback = std::function<void()>;

/**
 * Timer type
 */
enum class TimerType {
    ONCE,      // One-time timer
    REPEAT,    // Repeat timer
    CRON       // Cron expression timer
};

/**
 * Timer status
 */
enum class TimerStatus {
    PENDING,   // Waiting to execute
    RUNNING,   // Currently executing
    COMPLETED, // Completed
    CANCELLED, // Cancelled
    ERROR      // Execution error
};

/**
 * Timer information
 */
struct TimerInfo {
    std::string id;
    TimerType type;
    TimerStatus status;
    std::chrono::system_clock::time_point nextExecution;
    std::chrono::milliseconds interval;
    std::string cronExpression;
    TimerCallback callback;
    int repeatCount;        // 重复次数，-1表示无限
    int executedCount;      // 已执行次数
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastExecuted;
    
    TimerInfo() : type(TimerType::ONCE), status(TimerStatus::PENDING), 
                  repeatCount(0), executedCount(0) {}
};

/**
 * 定时器管理器
 * 支持一次性定时器、重复定时器和Cron表达式定时器
 */
class TimerManager {
public:
    TimerManager();
    ~TimerManager();
    
    /**
     * 启动定时器管理器
     */
    void start();
    
    /**
     * 停止定时器管理器
     */
    void stop();
    
    /**
     * 创建一次性定时器
     * @param id 定时器ID
     * @param delay 延迟时间（毫秒）
     * @param callback 回调函数
     * @return 是否创建成功
     */
    bool createOnceTimer(const std::string& id, 
                        std::chrono::milliseconds delay, 
                        TimerCallback callback);
    
    /**
     * 创建重复定时器
     * @param id 定时器ID
     * @param interval 间隔时间（毫秒）
     * @param callback 回调函数
     * @param repeatCount 重复次数，-1表示无限
     * @return 是否创建成功
     */
    bool createRepeatTimer(const std::string& id, 
                          std::chrono::milliseconds interval, 
                          TimerCallback callback,
                          int repeatCount = -1);
    
    /**
     * 创建Cron表达式定时器
     * @param id 定时器ID
     * @param cronExpression Cron表达式
     * @param callback 回调函数
     * @return 是否创建成功
     */
    bool createCronTimer(const std::string& id, 
                        const std::string& cronExpression, 
                        TimerCallback callback);
    
    /**
     * 取消定时器
     * @param id 定时器ID
     * @return 是否取消成功
     */
    bool cancelTimer(const std::string& id);
    
    /**
     * 获取定时器信息
     * @param id 定时器ID
     * @return 定时器信息，如果不存在则返回nullptr
     */
    std::shared_ptr<TimerInfo> getTimerInfo(const std::string& id);
    
    /**
     * 获取所有定时器信息
     * @return 所有定时器信息列表
     */
    std::vector<std::shared_ptr<TimerInfo>> getAllTimers();
    
    /**
     * 检查定时器是否存在
     * @param id 定时器ID
     * @return 是否存在
     */
    bool hasTimer(const std::string& id);
    
    /**
     * 清空所有定时器
     */
    void clearAllTimers();
    
    /**
     * 获取定时器统计信息
     */
    struct TimerStats {
        int totalTimers = 0;
        int activeTimers = 0;
        int completedTimers = 0;
        int cancelledTimers = 0;
        int errorTimers = 0;
        int totalExecutions = 0;
    };
    
    TimerStats getStats();

private:
    std::vector<std::shared_ptr<TimerInfo>> timers_;
    std::mutex timersMutex_;
    std::atomic<bool> running_;
    std::thread workerThread_;
    std::condition_variable cv_;
    
    /**
     * 工作线程函数
     */
    void workerLoop();
    
    /**
     * 执行定时器
     * @param timer 定时器信息
     */
    void executeTimer(std::shared_ptr<TimerInfo> timer);
    
    /**
     * 计算下一个执行时间
     * @param timer 定时器信息
     * @return 下一个执行时间
     */
    std::chrono::system_clock::time_point calculateNextExecution(std::shared_ptr<TimerInfo> timer);
    
    /**
     * 生成唯一ID
     * @return 唯一ID
     */
    std::string generateId();
};

} // namespace scheduler
} // namespace snipper
