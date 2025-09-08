#include "timer.h"
#include "cron_parser.h"
#include <algorithm>
#include <random>
#include <sstream>

namespace snipper {
namespace scheduler {

TimerManager::TimerManager() : running_(false) {
}

TimerManager::~TimerManager() {
    stop();
}

void TimerManager::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    workerThread_ = std::thread(&TimerManager::workerLoop, this);
}

void TimerManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    cv_.notify_all();
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

bool TimerManager::createOnceTimer(const std::string& id, 
                                  std::chrono::milliseconds delay, 
                                  TimerCallback callback) {
    if (hasTimer(id)) {
        return false;
    }
    
    auto timer = std::make_shared<TimerInfo>();
    timer->id = id;
    timer->type = TimerType::ONCE;
    timer->status = TimerStatus::PENDING;
    timer->interval = delay;
    timer->callback = callback;
    timer->repeatCount = 1;
    timer->executedCount = 0;
    timer->createdAt = std::chrono::system_clock::now();
    timer->nextExecution = timer->createdAt + delay;
    
    {
        std::lock_guard<std::mutex> lock(timersMutex_);
        timers_.push_back(timer);
    }
    
    cv_.notify_all();
    return true;
}

bool TimerManager::createRepeatTimer(const std::string& id, 
                                    std::chrono::milliseconds interval, 
                                    TimerCallback callback,
                                    int repeatCount) {
    if (hasTimer(id)) {
        return false;
    }
    
    auto timer = std::make_shared<TimerInfo>();
    timer->id = id;
    timer->type = TimerType::REPEAT;
    timer->status = TimerStatus::PENDING;
    timer->interval = interval;
    timer->callback = callback;
    timer->repeatCount = repeatCount;
    timer->executedCount = 0;
    timer->createdAt = std::chrono::system_clock::now();
    timer->nextExecution = timer->createdAt + interval;
    
    {
        std::lock_guard<std::mutex> lock(timersMutex_);
        timers_.push_back(timer);
    }
    
    cv_.notify_all();
    return true;
}

bool TimerManager::createCronTimer(const std::string& id, 
                                  const std::string& cronExpression, 
                                  TimerCallback callback) {
    if (hasTimer(id)) {
        return false;
    }
    
    // 验证cron表达式
    if (!CronParser::isValid(cronExpression)) {
        return false;
    }
    
    auto timer = std::make_shared<TimerInfo>();
    timer->id = id;
    timer->type = TimerType::CRON;
    timer->status = TimerStatus::PENDING;
    timer->cronExpression = cronExpression;
    timer->callback = callback;
    timer->repeatCount = -1; // 无限重复
    timer->executedCount = 0;
    timer->createdAt = std::chrono::system_clock::now();
    timer->nextExecution = calculateNextExecution(timer);
    
    {
        std::lock_guard<std::mutex> lock(timersMutex_);
        timers_.push_back(timer);
    }
    
    cv_.notify_all();
    return true;
}

bool TimerManager::cancelTimer(const std::string& id) {
    std::lock_guard<std::mutex> lock(timersMutex_);
    
    auto it = std::find_if(timers_.begin(), timers_.end(),
                          [&id](const std::shared_ptr<TimerInfo>& timer) {
                              return timer->id == id;
                          });
    
    if (it != timers_.end()) {
        (*it)->status = TimerStatus::CANCELLED;
        return true;
    }
    
    return false;
}

std::shared_ptr<TimerInfo> TimerManager::getTimerInfo(const std::string& id) {
    std::lock_guard<std::mutex> lock(timersMutex_);
    
    auto it = std::find_if(timers_.begin(), timers_.end(),
                          [&id](const std::shared_ptr<TimerInfo>& timer) {
                              return timer->id == id;
                          });
    
    if (it != timers_.end()) {
        return *it;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<TimerInfo>> TimerManager::getAllTimers() {
    std::lock_guard<std::mutex> lock(timersMutex_);
    return timers_;
}

bool TimerManager::hasTimer(const std::string& id) {
    return getTimerInfo(id) != nullptr;
}

void TimerManager::clearAllTimers() {
    std::lock_guard<std::mutex> lock(timersMutex_);
    timers_.clear();
}

TimerManager::TimerStats TimerManager::getStats() {
    std::lock_guard<std::mutex> lock(timersMutex_);
    
    TimerStats stats;
    stats.totalTimers = timers_.size();
    
    for (const auto& timer : timers_) {
        switch (timer->status) {
            case TimerStatus::PENDING:
            case TimerStatus::RUNNING:
                stats.activeTimers++;
                break;
            case TimerStatus::COMPLETED:
                stats.completedTimers++;
                break;
            case TimerStatus::CANCELLED:
                stats.cancelledTimers++;
                break;
            case TimerStatus::ERROR:
                stats.errorTimers++;
                break;
        }
        stats.totalExecutions += timer->executedCount;
    }
    
    return stats;
}

void TimerManager::workerLoop() {
    while (running_) {
        std::vector<std::shared_ptr<TimerInfo>> timersToExecute;
        
        // 获取需要执行的定时器
        {
            std::lock_guard<std::mutex> lock(timersMutex_);
            auto now = std::chrono::system_clock::now();
            
            for (auto& timer : timers_) {
                if (timer->status == TimerStatus::PENDING && 
                    timer->nextExecution <= now) {
                    timersToExecute.push_back(timer);
                }
            }
        }
        
        // 执行定时器
        for (auto& timer : timersToExecute) {
            executeTimer(timer);
        }
        
        // 清理已完成的定时器
        {
            std::lock_guard<std::mutex> lock(timersMutex_);
            timers_.erase(
                std::remove_if(timers_.begin(), timers_.end(),
                              [](const std::shared_ptr<TimerInfo>& timer) {
                                  return timer->status == TimerStatus::COMPLETED ||
                                         timer->status == TimerStatus::CANCELLED;
                              }),
                timers_.end()
            );
        }
        
        // 等待下一个检查周期
        std::unique_lock<std::mutex> lock(timersMutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(100));
    }
}

void TimerManager::executeTimer(std::shared_ptr<TimerInfo> timer) {
    timer->status = TimerStatus::RUNNING;
    timer->lastExecuted = std::chrono::system_clock::now();
    
    try {
        if (timer->callback) {
            timer->callback();
        }
        timer->executedCount++;
        
        // 更新状态和下次执行时间
        if (timer->type == TimerType::ONCE) {
            timer->status = TimerStatus::COMPLETED;
        } else if (timer->type == TimerType::REPEAT) {
            if (timer->repeatCount == -1 || timer->executedCount < timer->repeatCount) {
                timer->status = TimerStatus::PENDING;
                timer->nextExecution = calculateNextExecution(timer);
            } else {
                timer->status = TimerStatus::COMPLETED;
            }
        } else if (timer->type == TimerType::CRON) {
            timer->status = TimerStatus::PENDING;
            timer->nextExecution = calculateNextExecution(timer);
        }
    } catch (...) {
        timer->status = TimerStatus::ERROR;
    }
}

std::chrono::system_clock::time_point TimerManager::calculateNextExecution(std::shared_ptr<TimerInfo> timer) {
    auto now = std::chrono::system_clock::now();
    
    if (timer->type == TimerType::ONCE || timer->type == TimerType::REPEAT) {
        return now + timer->interval;
    } else if (timer->type == TimerType::CRON) {
        auto cron = CronParser::parse(timer->cronExpression);
        return CronParser::nextMatch(cron, now);
    }
    
    return now;
}

std::string TimerManager::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    std::stringstream ss;
    ss << "timer_" << std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch()).count()
       << "_" << dis(gen);
    return ss.str();
}

} // namespace scheduler
} // namespace snipper
