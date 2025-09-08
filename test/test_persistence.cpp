#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include "runtime/persistence/persistence.h"

using namespace snipper::persistence;

int main() {
    std::cout << "=== 数据持久化和状态管理测试程序 ===" << std::endl;
    
    // 创建持久化管理器（使用内存存储）
    auto storage = StorageFactory::create("memory", nlohmann::json::object());
    PersistenceManager persistenceManager(std::move(storage));
    
    // 连接
    if (!persistenceManager.connect()) {
        std::cout << "错误: 无法连接到存储" << std::endl;
        return 1;
    }
    
    std::cout << "✓ 已连接到存储" << std::endl;
    
    // 测试规则状态管理
    std::cout << "\n=== 测试规则状态管理 ===" << std::endl;
    
    auto ruleStateManager = persistenceManager.getRuleStateManager();
    if (ruleStateManager) {
        // 注册规则
        nlohmann::json ruleConfig = {
            {"priority", 10},
            {"throttle_ms", 1000},
            {"mode", "REPEAT"}
        };
        
        ruleStateManager->registerRule("rule1", "温度监控规则", ruleConfig);
        ruleStateManager->registerRule("rule2", "湿度监控规则", ruleConfig);
        ruleStateManager->registerRule("rule3", "压力监控规则", ruleConfig);
        
        std::cout << "✓ 注册了3个规则" << std::endl;
        std::cout << "总规则数: " << ruleStateManager->getTotalRuleCount() << std::endl;
        std::cout << "启用规则数: " << ruleStateManager->getEnabledRuleCount() << std::endl;
        
        // 模拟规则执行
        ruleStateManager->recordRuleStart("rule1", {{"temperature", 25.5}});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ruleStateManager->recordRuleEnd("rule1", true);
        
        ruleStateManager->recordRuleStart("rule2", {{"humidity", 60.0}});
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ruleStateManager->recordRuleEnd("rule2", false, "湿度传感器故障");
        
        std::cout << "✓ 模拟了规则执行" << std::endl;
    }
    
    // 测试历史记录
    std::cout << "\n=== 测试历史记录 ===" << std::endl;
    
    auto historyRecorder = persistenceManager.getHistoryRecorder();
    if (historyRecorder) {
        // 记录传感器数据
        historyRecorder->recordSensorData("temp_sensor_1", "temperature", 25.5, "°C", "客厅");
        historyRecorder->recordSensorData("humidity_sensor_1", "humidity", 60.0, "%", "客厅");
        historyRecorder->recordSensorData("pressure_sensor_1", "pressure", 1013.25, "hPa", "室外");
        
        std::cout << "✓ 记录了传感器数据" << std::endl;
        std::cout << "传感器数据总数: " << historyRecorder->getSensorDataCount() << std::endl;
        std::cout << "规则执行总数: " << historyRecorder->getRuleExecutionCount() << std::endl;
        
        // 查询历史数据
        auto sensorHistory = historyRecorder->getSensorDataHistory("temp_sensor_1", 0, 10);
        std::cout << "温度传感器历史记录数: " << sensorHistory.records.size() << std::endl;
        
        auto ruleHistory = historyRecorder->getRuleExecutionHistory("rule1", 0, 10);
        std::cout << "rule1执行历史记录数: " << ruleHistory.records.size() << std::endl;
    }
    
    // 测试配置热重载
    std::cout << "\n=== 测试配置热重载 ===" << std::endl;
    
    auto configHotReload = persistenceManager.getConfigHotReload();
    if (configHotReload) {
        // 创建测试配置文件
        std::string configPath = "test_config.json";
        nlohmann::json testConfig = {
            {"rules", {
                {"rule1", {
                    {"enabled", true},
                    {"priority", 10}
                }},
                {"rule2", {
                    {"enabled", false},
                    {"priority", 5}
                }}
            }},
            {"sensors", {
                {"temperature", {"unit", "°C", "threshold", 30.0}},
                {"humidity", {"unit", "%", "threshold", 80.0}}
            }}
        };
        
        std::ofstream configFile(configPath);
        configFile << testConfig.dump(2);
        configFile.close();
        
        // 添加配置文件监控
        configHotReload->addConfigFile(configPath);
        
        // 添加变更回调
        configHotReload->addChangeCallback(configPath, [](const std::string& path, const nlohmann::json& config) {
            std::cout << "配置文件变更: " << path << std::endl;
        });
        
        std::cout << "✓ 添加了配置文件监控" << std::endl;
        
        // 启动监控
        configHotReload->startMonitoring();
        std::cout << "✓ 启动了配置监控" << std::endl;
        
        // 模拟配置文件变更
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        testConfig["rules"]["rule1"]["priority"] = 15;
        std::ofstream configFile2(configPath);
        configFile2 << testConfig.dump(2);
        configFile2.close();
        
        std::cout << "✓ 修改了配置文件" << std::endl;
        
        // 等待检测到变更
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        
        // 停止监控
        configHotReload->stopMonitoring();
        std::cout << "✓ 停止了配置监控" << std::endl;
        
        // 清理测试文件
        std::remove(configPath.c_str());
    }
    
    // 测试文件存储
    std::cout << "\n=== 测试文件存储 ===" << std::endl;
    
    auto fileStorage = StorageFactory::create("file", nlohmann::json{
        {"file_path", "test_data.json"},
        {"auto_save", true}
    });
    
    if (fileStorage && fileStorage->connect()) {
        // 插入测试数据
        DataRecord record("test1", "test_type", nlohmann::json{{"value", 42}}, "test_source");
        fileStorage->insert(record);
        
        // 查询数据
        auto result = fileStorage->queryByType("test_type");
        std::cout << "文件存储记录数: " << result.records.size() << std::endl;
        
        fileStorage->disconnect();
        std::cout << "✓ 文件存储测试完成" << std::endl;
        
        // 清理测试文件
        std::remove("test_data.json");
    }
    
    // 显示系统统计
    std::cout << "\n=== 系统统计 ===" << std::endl;
    auto stats = persistenceManager.getSystemStats();
    std::cout << "系统统计信息:" << std::endl;
    std::cout << stats.dump(2) << std::endl;
    
    // 显示规则统计
    if (ruleStateManager) {
        auto ruleStats = persistenceManager.getRuleStats("rule1");
        std::cout << "\nrule1统计信息:" << std::endl;
        std::cout << ruleStats.dump(2) << std::endl;
    }
    
    // 断开连接
    persistenceManager.disconnect();
    std::cout << "\n✓ 已断开连接" << std::endl;
    
    std::cout << "\n=== 数据持久化和状态管理测试完成 ===" << std::endl;
    
    return 0;
}
