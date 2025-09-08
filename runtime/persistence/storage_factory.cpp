#include "storage_interface.h"
#include "memory_storage.h"
#include "file_storage.h"

namespace snipper::persistence {

std::unique_ptr<StorageInterface> StorageFactory::create(StorageType type, const json& config) {
    switch (type) {
        case MEMORY:
            return std::make_unique<MemoryStorage>();
            
        case FILE: {
            std::string filePath = config.value("file_path", "data.json");
            bool autoSave = config.value("auto_save", true);
            return std::make_unique<FileStorage>(filePath, autoSave);
        }
        
        case SQLITE:
            // TODO: 实现SQLite存储
            return nullptr;
            
        case MYSQL:
            // TODO: 实现MySQL存储
            return nullptr;
            
        case POSTGRESQL:
            // TODO: 实现PostgreSQL存储
            return nullptr;
            
        default:
            return nullptr;
    }
}

std::unique_ptr<StorageInterface> StorageFactory::create(const std::string& type, const json& config) {
    if (type == "memory") {
        return create(MEMORY, config);
    } else if (type == "file") {
        return create(FILE, config);
    } else if (type == "sqlite") {
        return create(SQLITE, config);
    } else if (type == "mysql") {
        return create(MYSQL, config);
    } else if (type == "postgresql") {
        return create(POSTGRESQL, config);
    }
    
    return nullptr;
}

} // namespace snipper::persistence
