#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace snipper::persistence {

using json = nlohmann::json;

// 配置变更回调函数类型
using ConfigChangeCallback = std::function<void(const std::string& configPath, const json& newConfig)>;
using ConfigErrorCallback = std::function<void(const std::string& configPath, const std::string& error)>;

// 配置文件监控信息
struct ConfigFileInfo {
    std::string path;
    std::filesystem::file_time_type lastModified;
    json lastConfig;
    std::vector<ConfigChangeCallback> changeCallbacks;
    std::vector<ConfigErrorCallback> errorCallbacks;
    bool isValid;
    std::string lastError;
    
    ConfigFileInfo() = default;
    ConfigFileInfo(const std::string& path) : path(path), isValid(false) {
        lastModified = std::filesystem::file_time_type::min();
    }
};

// 配置热重载管理器
class ConfigHotReload {
private:
    std::unordered_map<std::string, ConfigFileInfo> configFiles_;
    std::thread monitorThread_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;
    mutable std::mutex mutex_;
    std::chrono::milliseconds checkInterval_;
    
    // 全局回调
    std::vector<ConfigChangeCallback> globalChangeCallbacks_;
    std::vector<ConfigErrorCallback> globalErrorCallbacks_;
    
public:
    explicit ConfigHotReload(std::chrono::milliseconds checkInterval = std::chrono::milliseconds(1000));
    ~ConfigHotReload();
    
    // 配置文件管理
    bool addConfigFile(const std::string& configPath);
    bool removeConfigFile(const std::string& configPath);
    bool reloadConfigFile(const std::string& configPath);
    bool reloadAllConfigFiles();
    
    // 回调管理
    bool addChangeCallback(const std::string& configPath, ConfigChangeCallback callback);
    bool addErrorCallback(const std::string& configPath, ConfigErrorCallback callback);
    bool addGlobalChangeCallback(ConfigChangeCallback callback);
    bool addGlobalErrorCallback(ConfigErrorCallback callback);
    bool removeChangeCallback(const std::string& configPath, ConfigChangeCallback callback);
    bool removeErrorCallback(const std::string& configPath, ConfigErrorCallback callback);
    
    // 配置查询
    json getConfig(const std::string& configPath) const;
    std::vector<std::string> getConfigPaths() const;
    bool isConfigValid(const std::string& configPath) const;
    std::string getLastError(const std::string& configPath) const;
    
    // 监控控制
    bool startMonitoring();
    bool stopMonitoring();
    bool isMonitoring() const;
    
    // 手动检查
    bool checkForChanges();
    
    // 配置验证
    bool validateConfig(const json& config) const;
    bool setConfigValidator(std::function<bool(const json&)> validator);
    
private:
    void monitorLoop();
    bool loadConfigFile(const std::string& configPath, json& config);
    bool saveConfigFile(const std::string& configPath, const json& config);
    void notifyConfigChange(const std::string& configPath, const json& newConfig);
    void notifyConfigError(const std::string& configPath, const std::string& error);
    std::filesystem::file_time_type getFileModificationTime(const std::string& path) const;
    
    // 配置验证器
    std::function<bool(const json&)> configValidator_;
};

} // namespace snipper::persistence
