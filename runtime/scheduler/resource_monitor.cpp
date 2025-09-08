#include "resource_monitor.h"
#include <algorithm>

namespace snipper {
namespace scheduler {

ResourceMonitor::ResourceMonitor() {
}

ResourceMonitor::~ResourceMonitor() {
}

void ResourceMonitor::startMonitoring(const std::string& identifier, const ResourceLimit& limits) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto data = std::make_shared<MonitoringData>();
    data->limits = limits;
    monitoringData_[identifier] = data;
}

void ResourceMonitor::stopMonitoring(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    monitoringData_.erase(identifier);
}

void ResourceMonitor::recordUsage(const std::string& identifier, 
                                 size_t memoryDelta,
                                 size_t executionTimeMs,
                                 bool success) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = monitoringData_.find(identifier);
    if (it == monitoringData_.end()) {
        return;
    }
    
    auto& data = it->second;
    auto now = std::chrono::system_clock::now();
    
    // 更新内存使用量
    if (memoryDelta != 0) {
        data->usage.memoryUsage += memoryDelta;
    }
    
    // 更新CPU时间
    data->usage.cpuTimeMs += executionTimeMs;
    
    // 更新执行次数
    data->usage.executionCount++;
    
    // 更新错误次数
    if (!success) {
        data->usage.errorCount++;
    }
    
    // 更新总执行时间
    data->usage.totalExecutionTimeMs += executionTimeMs;
    
    // 更新最后更新时间
    data->usage.lastUpdate = now;
}

ResourceStatus ResourceMonitor::getResourceStatus(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ResourceStatus status;
    
    auto it = monitoringData_.find(identifier);
    if (it == monitoringData_.end()) {
        status.withinLimits = false;
        status.violationReason = "Not monitored";
        return status;
    }
    
    const auto& data = it->second;
    status.currentUsage = data->usage;
    status.limits = data->limits;
    
    // 计算使用百分比
    if (data->limits.maxMemoryUsage > 0) {
        status.memoryUsagePercent = calculateUsagePercent(
            data->usage.memoryUsage.load(), data->limits.maxMemoryUsage);
    }
    
    if (data->limits.maxCpuTimeMs > 0) {
        status.cpuUsagePercent = calculateUsagePercent(
            data->usage.cpuTimeMs.load(), data->limits.maxCpuTimeMs);
    }
    
    // 计算错误率
    if (data->usage.executionCount.load() > 0) {
        status.errorRate = static_cast<double>(data->usage.errorCount.load()) 
                          / data->usage.executionCount.load();
    }
    
    // 计算平均执行时间
    if (data->usage.executionCount.load() > 0) {
        status.averageExecutionTimeMs = data->usage.totalExecutionTimeMs.load() 
                                      / data->usage.executionCount.load();
    }
    
    // 检查限制
    status.withinLimits = checkLimits(identifier);
    if (!status.withinLimits) {
        status.violationReason = checkViolations(*data);
    }
    
    return status;
}

bool ResourceMonitor::checkLimits(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = monitoringData_.find(identifier);
    if (it == monitoringData_.end()) {
        return false;
    }
    
    const auto& data = it->second;
    return checkViolations(*data).empty();
}

void ResourceMonitor::setLimits(const std::string& identifier, const ResourceLimit& limits) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = monitoringData_.find(identifier);
    if (it != monitoringData_.end()) {
        it->second->limits = limits;
    }
}

void ResourceMonitor::reset(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (identifier.empty()) {
        monitoringData_.clear();
    } else {
        auto it = monitoringData_.find(identifier);
        if (it != monitoringData_.end()) {
            it->second->usage = ResourceUsage();
        }
    }
}

std::vector<std::string> ResourceMonitor::getMonitoredIdentifiers() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> identifiers;
    for (const auto& pair : monitoringData_) {
        identifiers.push_back(pair.first);
    }
    
    return identifiers;
}

ResourceMonitor::GlobalStats ResourceMonitor::getGlobalStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GlobalStats stats;
    stats.monitoredCount = monitoringData_.size();
    
    for (const auto& pair : monitoringData_) {
        const auto& data = pair.second;
        stats.totalMemoryUsage += data->usage.memoryUsage.load();
        stats.totalCpuTimeMs += data->usage.cpuTimeMs.load();
        stats.totalExecutions += data->usage.executionCount.load();
        stats.totalErrors += data->usage.errorCount.load();
    }
    
    if (stats.totalExecutions > 0) {
        stats.averageErrorRate = static_cast<double>(stats.totalErrors) / stats.totalExecutions;
    }
    
    return stats;
}

void ResourceMonitor::cleanupExpiredData(int maxAgeHours) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto cutoffTime = std::chrono::system_clock::now() - std::chrono::hours(maxAgeHours);
    
    auto it = monitoringData_.begin();
    while (it != monitoringData_.end()) {
        if (it->second->usage.lastUpdate < cutoffTime) {
            it = monitoringData_.erase(it);
        } else {
            ++it;
        }
    }
}

double ResourceMonitor::calculateUsagePercent(size_t current, size_t limit) {
    if (limit == 0) {
        return 0.0;
    }
    
    return std::min(100.0, static_cast<double>(current) / limit * 100.0);
}

std::string ResourceMonitor::checkViolations(const MonitoringData& data) {
    const auto& usage = data.usage;
    const auto& limits = data.limits;
    
    // 检查内存使用限制
    if (limits.maxMemoryUsage > 0 && usage.memoryUsage.load() > limits.maxMemoryUsage) {
        return "Memory usage exceeds limit";
    }
    
    // 检查CPU时间限制
    if (limits.maxCpuTimeMs > 0 && usage.cpuTimeMs.load() > limits.maxCpuTimeMs) {
        return "CPU time exceeds limit";
    }
    
    // 检查执行次数限制
    if (limits.maxExecutionCount > 0 && usage.executionCount.load() > limits.maxExecutionCount) {
        return "Execution count exceeds limit";
    }
    
    // 检查错误率限制
    if (limits.maxErrorRate > 0 && usage.executionCount.load() > 0) {
        double errorRate = static_cast<double>(usage.errorCount.load()) / usage.executionCount.load();
        if (errorRate > limits.maxErrorRate) {
            return "Error rate exceeds limit";
        }
    }
    
    // 检查单次执行时间限制
    if (limits.maxExecutionTimeMs > 0 && usage.executionCount.load() > 0) {
        size_t averageExecutionTime = usage.totalExecutionTimeMs.load() / usage.executionCount.load();
        if (averageExecutionTime > limits.maxExecutionTimeMs) {
            return "Average execution time exceeds limit";
        }
    }
    
    return "";
}

} // namespace scheduler
} // namespace snipper
