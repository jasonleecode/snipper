#include <iostream>
#include <thread>
#include <chrono>
#include "runtime/scheduler/frequency_limiter.h"

using namespace snipper::scheduler;

int main() {
    std::cout << "=== 频率限制器测试 ===" << std::endl;
    
    FrequencyLimiter limiter;
    
    // 设置限制：每分钟最多3次请求
    FrequencyLimiter::LimitConfig config;
    config.maxRequests = 3;
    config.windowMs = std::chrono::minutes(1);
    config.strategy = FrequencyLimiter::Strategy::SLIDING_WINDOW;
    
    limiter.setLimit("test_rule", config);
    
    std::cout << "设置限制：每分钟最多3次请求" << std::endl;
    std::cout << "开始测试..." << std::endl;
    
    // 模拟请求
    for (int i = 0; i < 8; ++i) {
        auto result = limiter.checkLimit("test_rule");
        std::cout << "请求 " << (i + 1) << ": " 
                  << (result.allowed ? "允许" : "拒绝")
                  << ", 剩余: " << result.remainingRequests << std::endl;
        
        if (result.allowed) {
            std::cout << "  -> 处理请求..." << std::endl;
        } else {
            std::cout << "  -> 请求被限制" << std::endl;
        }
        
        // 短暂延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto stats = limiter.getStats();
    std::cout << "\n频率限制统计:" << std::endl;
    std::cout << "总请求: " << stats.totalRequests << std::endl;
    std::cout << "被阻止: " << stats.blockedRequests << std::endl;
    std::cout << "阻止率: " << (stats.blockRate * 100) << "%" << std::endl;
    
    return 0;
}
