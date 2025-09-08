#include "bt_executor.h"
#include <iostream>
#include <chrono>

// BTExecutor 实现
BTExecutor::BTExecutor() 
    : current_status_(BTStatus::FAILURE), is_running_(false), is_paused_(false),
      execution_count_(0), success_count_(0), failure_count_(0), running_count_(0) {
}

void BTExecutor::setRoot(shared_ptr<BTNode> root) {
    root_ = root;
    reset();
}

BTStatus BTExecutor::execute(Context& ctx) {
    if (!root_) {
        return BTStatus::FAILURE;
    }
    
    if (is_paused_) {
        return current_status_;
    }
    
    is_running_ = true;
    current_status_ = root_->execute(ctx);
    updateStats(current_status_);
    
    // 如果执行完成（成功或失败），停止运行
    if (current_status_ != BTStatus::RUNNING) {
        is_running_ = false;
    }
    
    return current_status_;
}

void BTExecutor::reset() {
    if (root_) {
        root_->reset();
    }
    current_status_ = BTStatus::FAILURE;
    is_running_ = false;
    is_paused_ = false;
}

void BTExecutor::pause() {
    is_paused_ = true;
}

void BTExecutor::resume() {
    is_paused_ = false;
}

void BTExecutor::stop() {
    is_running_ = false;
    is_paused_ = false;
    current_status_ = BTStatus::FAILURE;
}

bool BTExecutor::isRunning() const {
    return is_running_ && !is_paused_;
}

BTStatus BTExecutor::getStatus() const {
    return current_status_;
}

void BTExecutor::registerAction(const string& name, BTAction::ActionFunction func) {
    action_functions_[name] = func;
}

void BTExecutor::registerCondition(const string& name, BTCondition::ConditionFunction func) {
    condition_functions_[name] = func;
}

json BTExecutor::getTreeInfo() const {
    if (!root_) {
        return json::object();
    }
    
    json info;
    info["root"] = root_->getInfo();
    info["is_running"] = is_running_;
    info["is_paused"] = is_paused_;
    info["current_status"] = static_cast<int>(current_status_);
    return info;
}

json BTExecutor::getExecutionStats() const {
    json stats;
    stats["execution_count"] = execution_count_;
    stats["success_count"] = success_count_;
    stats["failure_count"] = failure_count_;
    stats["running_count"] = running_count_;
    
    if (execution_count_ > 0) {
        stats["success_rate"] = static_cast<double>(success_count_) / execution_count_;
        stats["failure_rate"] = static_cast<double>(failure_count_) / execution_count_;
        stats["running_rate"] = static_cast<double>(running_count_) / execution_count_;
    } else {
        stats["success_rate"] = 0.0;
        stats["failure_rate"] = 0.0;
        stats["running_rate"] = 0.0;
    }
    
    return stats;
}

void BTExecutor::updateStats(BTStatus status) {
    execution_count_++;
    
    switch (status) {
        case BTStatus::SUCCESS:
            success_count_++;
            break;
        case BTStatus::FAILURE:
            failure_count_++;
            break;
        case BTStatus::RUNNING:
            running_count_++;
            break;
    }
}
