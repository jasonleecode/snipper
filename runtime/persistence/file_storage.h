#pragma once

#include "storage_interface.h"
#include <fstream>
#include <mutex>
#include <unordered_map>

namespace snipper::persistence {

class FileStorage : public StorageInterface {
private:
    std::string filePath_;
    std::unordered_map<std::string, DataRecord> records_;
    mutable std::mutex mutex_;
    bool connected_;
    bool inTransaction_;
    bool autoSave_;
    
    // 文件操作
    bool loadFromFile();
    bool saveToFile();
    std::string generateId() const;
    
public:
    explicit FileStorage(const std::string& filePath, bool autoSave = true);
    virtual ~FileStorage();
    
    // 基本CRUD操作
    bool insert(const DataRecord& record) override;
    bool update(const std::string& id, const json& data) override;
    bool remove(const std::string& id) override;
    DataRecord findById(const std::string& id) override;
    
    // 查询操作
    QueryResult query(const std::vector<QueryCondition>& conditions, 
                     size_t offset = 0, size_t limit = 100) override;
    QueryResult queryByType(const std::string& type, 
                           size_t offset = 0, size_t limit = 100) override;
    QueryResult queryByTimeRange(const std::chrono::system_clock::time_point& start,
                                const std::chrono::system_clock::time_point& end,
                                size_t offset = 0, size_t limit = 100) override;
    
    // 统计操作
    size_t count(const std::vector<QueryCondition>& conditions) override;
    size_t countByType(const std::string& type) override;
    
    // 清理操作
    bool cleanup(const std::chrono::system_clock::time_point& before) override;
    bool cleanupByType(const std::string& type, 
                      const std::chrono::system_clock::time_point& before) override;
    
    // 连接管理
    bool connect() override;
    bool disconnect() override;
    bool isConnected() const override;
    
    // 事务支持
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    
    // 文件存储特有方法
    bool setAutoSave(bool enabled);
    bool forceSave();
    bool backup(const std::string& backupPath) const;
    bool restore(const std::string& backupPath);
    
private:
    bool matchesConditions(const DataRecord& record, const std::vector<QueryCondition>& conditions) const;
    bool matchesCondition(const DataRecord& record, const QueryCondition& condition) const;
    bool compareValues(const json& recordValue, const std::string& op, const json& conditionValue) const;
};

} // namespace snipper::persistence
