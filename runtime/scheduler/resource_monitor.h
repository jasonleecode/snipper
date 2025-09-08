#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <atomic>
#include <memory>
#include <vector>

namespace snipper {
namespace scheduler {

/**
 * 资源使用统计
 */
struct ResourceUsage {
    std::atomic<size_t> memoryUsage{0};        // Memory usage (bytes)
    std::atomic<size_t> cpuTimeMs{0};          // CPU time (milliseconds)
    std::atomic<size_t> executionCount{0};     // Execution count
    std::atomic<size_t> errorCount{0};         // Error count
    std::atomic<size_t> totalExecutionTimeMs{0}; // Total execution time (milliseconds)
    
    std::chrono::system_clock::time_point lastUpdate;
    std::chrono::system_clock::time_point startTime;
    
    ResourceUsage() : lastUpdate(std::chrono::system_clock::now()),
                     startTime(std::chrono::system_clock::now()) {}
    
    // Copy constructor
    ResourceUsage(const ResourceUsage& other) 
        : memoryUsage(other.memoryUsage.load()),
          cpuTimeMs(other.cpuTimeMs.load()),
          executionCount(other.executionCount.load()),
          errorCount(other.errorCount.load()),
          totalExecutionTimeMs(other.totalExecutionTimeMs.load()),
          lastUpdate(other.lastUpdate),
          startTime(other.startTime) {}
    
    // Assignment operator
    ResourceUsage& operator=(const ResourceUsage& other) {
        if (this != &other) {
            memoryUsage.store(other.memoryUsage.load());
            cpuTimeMs.store(other.cpuTimeMs.load());
            executionCount.store(other.executionCount.load());
            errorCount.store(other.errorCount.load());
            totalExecutionTimeMs.store(other.totalExecutionTimeMs.load());
            lastUpdate = other.lastUpdate;
            startTime = other.startTime;
        }
        return *this;
    }
};

/**
 * 资源限制配置
 */
struct ResourceLimit {
    size_t maxMemoryUsage = 0;        // 最大内存使用量（字节），0表示无限制
    size_t maxCpuTimeMs = 0;          // 最大CPU时间（毫秒），0表示无限制
    size_t maxExecutionCount = 0;     // 最大执行次数，0表示无限制
    double maxErrorRate = 0.0;        // 最大错误率（0.0-1.0），0表示无限制
    size_t maxExecutionTimeMs = 0;    // 最大单次执行时间（毫秒），0表示无限制
    
    ResourceLimit() = default;
};

/**
 * 资源监控结果
 */
struct ResourceStatus {
    bool withinLimits = true;         // Whether within limits
    std::string violationReason;      // Violation reason
    double memoryUsagePercent = 0.0;  // Memory usage percentage
    double cpuUsagePercent = 0.0;     // CPU usage percentage
    double errorRate = 0.0;           // Error rate
    size_t averageExecutionTimeMs = 0; // Average execution time (milliseconds)
    
    ResourceUsage currentUsage;       // Current usage
    ResourceLimit limits;             // Limit configuration
    
    // Copy constructor
    ResourceStatus(const ResourceStatus& other) 
        : withinLimits(other.withinLimits),
          violationReason(other.violationReason),
          memoryUsagePercent(other.memoryUsagePercent),
          cpuUsagePercent(other.cpuUsagePercent),
          errorRate(other.errorRate),
          averageExecutionTimeMs(other.averageExecutionTimeMs),
          currentUsage(other.currentUsage),
          limits(other.limits) {}
    
    // Default constructor
    ResourceStatus() = default;
    
    // Assignment operator
    ResourceStatus& operator=(const ResourceStatus& other) {
        if (this != &other) {
            withinLimits = other.withinLimits;
            violationReason = other.violationReason;
            memoryUsagePercent = other.memoryUsagePercent;
            cpuUsagePercent = other.cpuUsagePercent;
            errorRate = other.errorRate;
            averageExecutionTimeMs = other.averageExecutionTimeMs;
            currentUsage = other.currentUsage;
            limits = other.limits;
        }
        return *this;
    }
};

/**
 * 资源监控器
 * 监控和管理系统资源使用情况
 */
class ResourceMonitor {
public:
    ResourceMonitor();
    ~ResourceMonitor();

    /**
     * 开始监控资源使用
     * @param identifier 标识符（如规则ID、任务ID等）
     * @param limits 资源限制配置
     */
    void startMonitoring(const std::string& identifier, const ResourceLimit& limits = ResourceLimit());

    /**
     * 停止监控资源使用
     * @param identifier 标识符
     */
    void stopMonitoring(const std::string& identifier);

    /**
     * 记录资源使用
     * @param identifier 标识符
     * @param memoryDelta 内存变化量（字节）
     * @param executionTimeMs 执行时间（毫秒）
     * @param success 是否成功执行
     */
    void recordUsage(const std::string& identifier, 
                    size_t memoryDelta = 0,
                    size_t executionTimeMs = 0,
                    bool success = true);

    /**
     * 获取资源状态
     * @param identifier 标识符
     * @return 资源状态
     */
    ResourceStatus getResourceStatus(const std::string& identifier);

    /**
     * 检查资源限制
     * @param identifier 标识符
     * @return 是否在限制范围内
     */
    bool checkLimits(const std::string& identifier);

    /**
     * 设置资源限制
     * @param identifier 标识符
     * @param limits 资源限制配置
     */
    void setLimits(const std::string& identifier, const ResourceLimit& limits);

    /**
     * 重置资源统计
     * @param identifier 标识符，为空则重置所有
     */
    void reset(const std::string& identifier = "");

    /**
     * 获取所有监控的标识符
     * @return 标识符列表
     */
    std::vector<std::string> getMonitoredIdentifiers();

    /**
     * 获取全局资源统计
     */
    struct GlobalStats {
        size_t totalMemoryUsage = 0;
        size_t totalCpuTimeMs = 0;
        size_t totalExecutions = 0;
        size_t totalErrors = 0;
        double averageErrorRate = 0.0;
        size_t monitoredCount = 0;
    };

    GlobalStats getGlobalStats();

    /**
     * 清理过期的监控数据
     * @param maxAgeHours 最大保留时间（小时）
     */
    void cleanupExpiredData(int maxAgeHours = 24);

private:
    struct MonitoringData {
        ResourceUsage usage;
        ResourceLimit limits;
        std::chrono::system_clock::time_point createdTime;
        
        MonitoringData() : createdTime(std::chrono::system_clock::now()) {}
    };

    std::map<std::string, std::shared_ptr<MonitoringData>> monitoringData_;
    std::mutex mutex_;

    /**
     * 计算资源使用百分比
     * @param current 当前使用量
     * @param limit 限制值
     * @return 使用百分比
     */
    double calculateUsagePercent(size_t current, size_t limit);

    /**
     * 检查是否违反限制
     * @param data 监控数据
     * @return 违反限制的原因，空字符串表示未违反
     */
    std::string checkViolations(const MonitoringData& data);

    /**
     * 更新全局统计
     */
    void updateGlobalStats();
};

} // namespace scheduler
} // namespace snipper
