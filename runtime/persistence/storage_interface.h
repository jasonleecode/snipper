#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace snipper::persistence {

using json = nlohmann::json;

// 数据记录结构
struct DataRecord {
    std::string id;
    std::string type;
    json data;
    std::chrono::system_clock::time_point timestamp;
    std::string source;
    
    DataRecord() = default;
    DataRecord(const std::string& id, const std::string& type, const json& data, 
               const std::string& source = "")
        : id(id), type(type), data(data), timestamp(std::chrono::system_clock::now()), source(source) {}
};

// 查询条件
struct QueryCondition {
    std::string field;
    std::string operator_;
    json value;
    std::string logic = "AND"; // AND, OR
    
    QueryCondition() = default;
    QueryCondition(const std::string& field, const std::string& op, const json& value, 
                   const std::string& logic = "AND")
        : field(field), operator_(op), value(value), logic(logic) {}
};

// 查询结果
struct QueryResult {
    std::vector<DataRecord> records;
    size_t totalCount;
    size_t offset;
    size_t limit;
    
    QueryResult() : totalCount(0), offset(0), limit(0) {}
};

// 存储接口抽象类
class StorageInterface {
public:
    virtual ~StorageInterface() = default;
    
    // 基本CRUD操作
    virtual bool insert(const DataRecord& record) = 0;
    virtual bool update(const std::string& id, const json& data) = 0;
    virtual bool remove(const std::string& id) = 0;
    virtual DataRecord findById(const std::string& id) = 0;
    
    // 查询操作
    virtual QueryResult query(const std::vector<QueryCondition>& conditions, 
                             size_t offset = 0, size_t limit = 100) = 0;
    virtual QueryResult queryByType(const std::string& type, 
                                   size_t offset = 0, size_t limit = 100) = 0;
    virtual QueryResult queryByTimeRange(const std::chrono::system_clock::time_point& start,
                                        const std::chrono::system_clock::time_point& end,
                                        size_t offset = 0, size_t limit = 100) = 0;
    
    // 统计操作
    virtual size_t count(const std::vector<QueryCondition>& conditions) = 0;
    virtual size_t countByType(const std::string& type) = 0;
    
    // 清理操作
    virtual bool cleanup(const std::chrono::system_clock::time_point& before) = 0;
    virtual bool cleanupByType(const std::string& type, 
                              const std::chrono::system_clock::time_point& before) = 0;
    
    // 连接管理
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    // 事务支持
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;
};

// 存储工厂
class StorageFactory {
public:
    enum StorageType {
        MEMORY,
        FILE,
        SQLITE,
        MYSQL,
        POSTGRESQL
    };
    
    static std::unique_ptr<StorageInterface> create(StorageType type, const json& config);
    static std::unique_ptr<StorageInterface> create(const std::string& type, const json& config);
};

} // namespace snipper::persistence
