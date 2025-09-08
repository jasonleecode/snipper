#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <deque>

namespace snipper {
namespace scheduler {

/**
 * 频率限制器
 * 支持基于时间窗口的频率限制
 */
class FrequencyLimiter {
public:
    /**
     * 频率限制策略
     */
    enum class Strategy {
        FIXED_WINDOW,    // 固定时间窗口
        SLIDING_WINDOW,  // 滑动时间窗口
        TOKEN_BUCKET     // 令牌桶算法
    };

    /**
     * 频率限制配置
     */
    struct LimitConfig {
        int maxRequests;                    // 最大请求数
        std::chrono::milliseconds windowMs; // 时间窗口（毫秒）
        Strategy strategy;                  // 限制策略
        
        LimitConfig() : maxRequests(100), windowMs(std::chrono::seconds(60)), 
                       strategy(Strategy::SLIDING_WINDOW) {}
    };

    /**
     * 请求记录
     */
    struct RequestRecord {
        std::chrono::system_clock::time_point timestamp;
        std::string identifier;
        
        RequestRecord(const std::string& id) 
            : timestamp(std::chrono::system_clock::now()), identifier(id) {}
    };

    /**
     * 频率限制结果
     */
    struct LimitResult {
        bool allowed;           // 是否允许
        int remainingRequests;  // 剩余请求数
        std::chrono::milliseconds resetAfter; // 重置时间
        std::chrono::system_clock::time_point nextReset; // 下次重置时间
        
        LimitResult() : allowed(false), remainingRequests(0), resetAfter(0) {}
    };

    FrequencyLimiter();
    ~FrequencyLimiter();

    /**
     * 设置频率限制配置
     * @param identifier 标识符（如规则ID、用户ID等）
     * @param config 限制配置
     */
    void setLimit(const std::string& identifier, const LimitConfig& config);

    /**
     * 检查是否允许请求
     * @param identifier 标识符
     * @return 限制结果
     */
    LimitResult checkLimit(const std::string& identifier);

    /**
     * 尝试获取请求许可
     * @param identifier 标识符
     * @return 是否成功获取许可
     */
    bool tryAcquire(const std::string& identifier);

    /**
     * 获取当前限制状态
     * @param identifier 标识符
     * @return 限制结果
     */
    LimitResult getStatus(const std::string& identifier);

    /**
     * 重置限制器
     * @param identifier 标识符，为空则重置所有
     */
    void reset(const std::string& identifier = "");

    /**
     * 清理过期的请求记录
     * @param identifier 标识符，为空则清理所有
     */
    void cleanup(const std::string& identifier = "");

    /**
     * 获取统计信息
     */
    struct Stats {
        int totalIdentifiers = 0;
        int totalRequests = 0;
        int blockedRequests = 0;
        double blockRate = 0.0;
    };

    Stats getStats();

private:
    struct IdentifierData {
        LimitConfig config;
        std::deque<RequestRecord> requests;
        std::chrono::system_clock::time_point lastCleanup;
        
        IdentifierData() : lastCleanup(std::chrono::system_clock::now()) {}
    };

    std::map<std::string, IdentifierData> identifierData_;
    std::mutex mutex_;
    Stats stats_;

    /**
     * 检查固定时间窗口限制
     * @param data 标识符数据
     * @param now 当前时间
     * @return 限制结果
     */
    LimitResult checkFixedWindow(IdentifierData& data, 
                                const std::chrono::system_clock::time_point& now);

    /**
     * 检查滑动时间窗口限制
     * @param data 标识符数据
     * @param now 当前时间
     * @return 限制结果
     */
    LimitResult checkSlidingWindow(IdentifierData& data, 
                                  const std::chrono::system_clock::time_point& now);

    /**
     * 检查令牌桶限制
     * @param data 标识符数据
     * @param now 当前时间
     * @return 限制结果
     */
    LimitResult checkTokenBucket(IdentifierData& data, 
                                const std::chrono::system_clock::time_point& now);

    /**
     * 清理过期的请求记录
     * @param data 标识符数据
     * @param now 当前时间
     */
    void cleanupExpiredRequests(IdentifierData& data, 
                               const std::chrono::system_clock::time_point& now);

    /**
     * 更新统计信息
     * @param allowed 是否允许请求
     */
    void updateStats(bool allowed);
};

} // namespace scheduler
} // namespace snipper
