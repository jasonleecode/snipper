#include "config_hot_reload.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace snipper::persistence {

ConfigHotReload::ConfigHotReload(std::chrono::milliseconds checkInterval)
    : running_(false), stopRequested_(false), checkInterval_(checkInterval) {
}

ConfigHotReload::~ConfigHotReload() {
    stopMonitoring();
}

bool ConfigHotReload::addConfigFile(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (configFiles_.find(configPath) != configFiles_.end()) {
        return false; // 已存在
    }
    
    ConfigFileInfo info(configPath);
    
    // 尝试加载配置文件
    json config;
    if (loadConfigFile(configPath, config)) {
        info.lastConfig = config;
        info.isValid = true;
        info.lastModified = getFileModificationTime(configPath);
    } else {
        info.isValid = false;
        info.lastError = "Failed to load config file";
    }
    
    configFiles_[configPath] = info;
    return true;
}

bool ConfigHotReload::removeConfigFile(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it != configFiles_.end()) {
        configFiles_.erase(it);
        return true;
    }
    
    return false;
}

bool ConfigHotReload::reloadConfigFile(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it == configFiles_.end()) {
        return false;
    }
    
    json newConfig;
    if (loadConfigFile(configPath, newConfig)) {
        it->second.lastConfig = newConfig;
        it->second.isValid = true;
        it->second.lastError = "";
        it->second.lastModified = getFileModificationTime(configPath);
        
        // 通知变更
        notifyConfigChange(configPath, newConfig);
        return true;
    } else {
        it->second.isValid = false;
        it->second.lastError = "Failed to reload config file";
        
        // 通知错误
        notifyConfigError(configPath, it->second.lastError);
        return false;
    }
}

bool ConfigHotReload::reloadAllConfigFiles() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool allSuccess = true;
    for (auto& pair : configFiles_) {
        if (!reloadConfigFile(pair.first)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool ConfigHotReload::addChangeCallback(const std::string& configPath, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it == configFiles_.end()) {
        return false;
    }
    
    it->second.changeCallbacks.push_back(callback);
    return true;
}

bool ConfigHotReload::addErrorCallback(const std::string& configPath, ConfigErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it == configFiles_.end()) {
        return false;
    }
    
    it->second.errorCallbacks.push_back(callback);
    return true;
}

bool ConfigHotReload::addGlobalChangeCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalChangeCallbacks_.push_back(callback);
    return true;
}

bool ConfigHotReload::addGlobalErrorCallback(ConfigErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalErrorCallbacks_.push_back(callback);
    return true;
}

bool ConfigHotReload::removeChangeCallback(const std::string& configPath, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it == configFiles_.end()) {
        return false;
    }
    
    // 由于std::function不能直接比较，我们无法精确删除特定的回调
    // 这里简化处理，清空所有回调
    it->second.changeCallbacks.clear();
    return true;
}

bool ConfigHotReload::removeErrorCallback(const std::string& configPath, ConfigErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it == configFiles_.end()) {
        return false;
    }
    
    // 由于std::function不能直接比较，我们无法精确删除特定的回调
    // 这里简化处理，清空所有回调
    it->second.errorCallbacks.clear();
    return true;
}

json ConfigHotReload::getConfig(const std::string& configPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it != configFiles_.end() && it->second.isValid) {
        return it->second.lastConfig;
    }
    
    return json::object();
}

std::vector<std::string> ConfigHotReload::getConfigPaths() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> paths;
    paths.reserve(configFiles_.size());
    
    for (const auto& pair : configFiles_) {
        paths.push_back(pair.first);
    }
    
    return paths;
}

bool ConfigHotReload::isConfigValid(const std::string& configPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    return it != configFiles_.end() && it->second.isValid;
}

std::string ConfigHotReload::getLastError(const std::string& configPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configFiles_.find(configPath);
    if (it != configFiles_.end()) {
        return it->second.lastError;
    }
    
    return "";
}

bool ConfigHotReload::startMonitoring() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return false;
    }
    
    stopRequested_ = false;
    running_ = true;
    
    monitorThread_ = std::thread(&ConfigHotReload::monitorLoop, this);
    return true;
}

bool ConfigHotReload::stopMonitoring() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return false;
        }
        
        stopRequested_ = true;
    }
    
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    
    running_ = false;
    return true;
}

bool ConfigHotReload::isMonitoring() const {
    return running_;
}

bool ConfigHotReload::checkForChanges() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool hasChanges = false;
    
    for (auto& pair : configFiles_) {
        const std::string& configPath = pair.first;
        ConfigFileInfo& info = pair.second;
        
        try {
            auto currentModTime = getFileModificationTime(configPath);
            
            if (currentModTime > info.lastModified) {
                json newConfig;
                if (loadConfigFile(configPath, newConfig)) {
                    info.lastConfig = newConfig;
                    info.isValid = true;
                    info.lastError = "";
                    info.lastModified = currentModTime;
                    
                    // 通知变更
                    notifyConfigChange(configPath, newConfig);
                    hasChanges = true;
                } else {
                    info.isValid = false;
                    info.lastError = "Failed to reload config file";
                    
                    // 通知错误
                    notifyConfigError(configPath, info.lastError);
                }
            }
        } catch (const std::exception& e) {
            info.isValid = false;
            info.lastError = std::string("Exception: ") + e.what();
            
            // 通知错误
            notifyConfigError(configPath, info.lastError);
        }
    }
    
    return hasChanges;
}

bool ConfigHotReload::validateConfig(const json& config) const {
    if (configValidator_) {
        return configValidator_(config);
    }
    
    // 默认验证：检查是否为有效的JSON对象
    return config.is_object() || config.is_array();
}

bool ConfigHotReload::setConfigValidator(std::function<bool(const json&)> validator) {
    std::lock_guard<std::mutex> lock(mutex_);
    configValidator_ = validator;
    return true;
}

void ConfigHotReload::monitorLoop() {
    while (!stopRequested_) {
        checkForChanges();
        
        std::this_thread::sleep_for(checkInterval_);
    }
}

bool ConfigHotReload::loadConfigFile(const std::string& configPath, json& config) {
    try {
        if (!std::filesystem::exists(configPath)) {
            return false;
        }
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            return false;
        }
        
        file >> config;
        
        // 验证配置
        if (!validateConfig(config)) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool ConfigHotReload::saveConfigFile(const std::string& configPath, const json& config) {
    try {
        // 确保目录存在
        std::filesystem::path path(configPath);
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }
        
        std::ofstream file(configPath);
        if (!file.is_open()) {
            return false;
        }
        
        file << config.dump(2);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void ConfigHotReload::notifyConfigChange(const std::string& configPath, const json& newConfig) {
    // 通知特定文件的回调
    auto it = configFiles_.find(configPath);
    if (it != configFiles_.end()) {
        for (const auto& callback : it->second.changeCallbacks) {
            try {
                callback(configPath, newConfig);
            } catch (const std::exception& e) {
                // 忽略回调异常
            }
        }
    }
    
    // 通知全局回调
    for (const auto& callback : globalChangeCallbacks_) {
        try {
            callback(configPath, newConfig);
        } catch (const std::exception& e) {
            // 忽略回调异常
        }
    }
}

void ConfigHotReload::notifyConfigError(const std::string& configPath, const std::string& error) {
    // 通知特定文件的错误回调
    auto it = configFiles_.find(configPath);
    if (it != configFiles_.end()) {
        for (const auto& callback : it->second.errorCallbacks) {
            try {
                callback(configPath, error);
            } catch (const std::exception& e) {
                // 忽略回调异常
            }
        }
    }
    
    // 通知全局错误回调
    for (const auto& callback : globalErrorCallbacks_) {
        try {
            callback(configPath, error);
        } catch (const std::exception& e) {
            // 忽略回调异常
        }
    }
}

std::filesystem::file_time_type ConfigHotReload::getFileModificationTime(const std::string& path) const {
    try {
        if (std::filesystem::exists(path)) {
            return std::filesystem::last_write_time(path);
        }
    } catch (const std::exception& e) {
        // 忽略异常
    }
    
    return std::filesystem::file_time_type::min();
}

} // namespace snipper::persistence
