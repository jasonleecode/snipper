#include "frequency_limiter.h"
#include <algorithm>

namespace snipper {
namespace scheduler {

FrequencyLimiter::FrequencyLimiter() {
}

FrequencyLimiter::~FrequencyLimiter() {
}

void FrequencyLimiter::setLimit(const std::string& identifier, const LimitConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    identifierData_[identifier].config = config;
}

FrequencyLimiter::LimitResult FrequencyLimiter::checkLimit(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = identifierData_.find(identifier);
    if (it == identifierData_.end()) {
        // 没有设置限制，默认允许
        LimitResult result;
        result.allowed = true;
        result.remainingRequests = -1; // 无限制
        return result;
    }
    
    auto& data = it->second;
    auto now = std::chrono::system_clock::now();
    
    // 清理过期请求
    cleanupExpiredRequests(data, now);
    
    LimitResult result;
    
    switch (data.config.strategy) {
        case Strategy::FIXED_WINDOW:
            result = checkFixedWindow(data, now);
            break;
        case Strategy::SLIDING_WINDOW:
            result = checkSlidingWindow(data, now);
            break;
        case Strategy::TOKEN_BUCKET:
            result = checkTokenBucket(data, now);
            break;
        default:
            result.allowed = true;
            break;
    }
    
    // 如果请求被允许，记录这个请求
    if (result.allowed) {
        data.requests.emplace_back(identifier);
    }
    
    updateStats(result.allowed);
    return result;
}

bool FrequencyLimiter::tryAcquire(const std::string& identifier) {
    auto result = checkLimit(identifier);
    return result.allowed;
}

FrequencyLimiter::LimitResult FrequencyLimiter::getStatus(const std::string& identifier) {
    return checkLimit(identifier);
}

void FrequencyLimiter::reset(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (identifier.empty()) {
        identifierData_.clear();
    } else {
        auto it = identifierData_.find(identifier);
        if (it != identifierData_.end()) {
            it->second.requests.clear();
        }
    }
}

void FrequencyLimiter::cleanup(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    
    if (identifier.empty()) {
        for (auto& pair : identifierData_) {
            cleanupExpiredRequests(pair.second, now);
        }
    } else {
        auto it = identifierData_.find(identifier);
        if (it != identifierData_.end()) {
            cleanupExpiredRequests(it->second, now);
        }
    }
}

FrequencyLimiter::Stats FrequencyLimiter::getStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

FrequencyLimiter::LimitResult FrequencyLimiter::checkFixedWindow(IdentifierData& data, 
                                                               const std::chrono::system_clock::time_point& now) {
    LimitResult result;
    
    // 计算当前时间窗口的开始时间
    auto windowStart = now - data.config.windowMs;
    
    // 计算当前窗口内的请求数
    int currentRequests = 0;
    for (const auto& request : data.requests) {
        if (request.timestamp >= windowStart) {
            currentRequests++;
        }
    }
    
    result.allowed = currentRequests < data.config.maxRequests;
    result.remainingRequests = std::max(0, data.config.maxRequests - currentRequests);
    
    // 计算重置时间（下一个窗口开始）
    auto nextWindowStart = now + data.config.windowMs;
    result.nextReset = nextWindowStart;
    result.resetAfter = std::chrono::duration_cast<std::chrono::milliseconds>(
        nextWindowStart - now);
    
    return result;
}

FrequencyLimiter::LimitResult FrequencyLimiter::checkSlidingWindow(IdentifierData& data, 
                                                                 const std::chrono::system_clock::time_point& now) {
    LimitResult result;
    
    // 计算滑动窗口的开始时间
    auto windowStart = now - data.config.windowMs;
    
    // 计算滑动窗口内的请求数
    int currentRequests = 0;
    for (const auto& request : data.requests) {
        if (request.timestamp >= windowStart) {
            currentRequests++;
        }
    }
    
    result.allowed = currentRequests < data.config.maxRequests;
    result.remainingRequests = std::max(0, data.config.maxRequests - currentRequests);
    
    // 计算重置时间（最早请求时间 + 窗口大小）
    if (!data.requests.empty()) {
        auto earliestRequest = data.requests.front().timestamp;
        result.nextReset = earliestRequest + data.config.windowMs;
        result.resetAfter = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.nextReset - now);
    } else {
        result.nextReset = now;
        result.resetAfter = std::chrono::milliseconds(0);
    }
    
    return result;
}

FrequencyLimiter::LimitResult FrequencyLimiter::checkTokenBucket(IdentifierData& data, 
                                                               const std::chrono::system_clock::time_point& now) {
    LimitResult result;
    
    // 简化的令牌桶实现
    // 这里假设每个请求消耗一个令牌，令牌按照固定速率补充
    
    // 计算应该补充的令牌数
    auto timeSinceLastCleanup = now - data.lastCleanup;
    auto tokensToAdd = std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceLastCleanup).count() 
                      * data.config.maxRequests / data.config.windowMs.count();
    
    // 更新清理时间
    data.lastCleanup = now;
    
    // 计算当前令牌数（简化实现）
    int currentTokens = std::min(data.config.maxRequests, 
                                static_cast<int>(tokensToAdd));
    
    result.allowed = currentTokens > 0;
    result.remainingRequests = currentTokens;
    
    // 计算下次补充时间
    auto tokensPerMs = static_cast<double>(data.config.maxRequests) / data.config.windowMs.count();
    auto timeToNextToken = tokensPerMs > 0 ? 
        std::chrono::milliseconds(static_cast<int>(1000 / tokensPerMs)) : 
        data.config.windowMs;
    
    result.nextReset = now + timeToNextToken;
    result.resetAfter = timeToNextToken;
    
    return result;
}

void FrequencyLimiter::cleanupExpiredRequests(IdentifierData& data, 
                                             const std::chrono::system_clock::time_point& now) {
    auto cutoffTime = now - data.config.windowMs;
    
    data.requests.erase(
        std::remove_if(data.requests.begin(), data.requests.end(),
                      [cutoffTime](const RequestRecord& request) {
                          return request.timestamp < cutoffTime;
                      }),
        data.requests.end()
    );
}

void FrequencyLimiter::updateStats(bool allowed) {
    stats_.totalRequests++;
    if (!allowed) {
        stats_.blockedRequests++;
    }
    stats_.blockRate = stats_.totalRequests > 0 ? 
        static_cast<double>(stats_.blockedRequests) / stats_.totalRequests : 0.0;
}

} // namespace scheduler
} // namespace snipper
