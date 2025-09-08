#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using namespace nlohmann;
using namespace std;

// 值类型，支持多种数据类型
using Value = json;

// 上下文类，存储传感器数据
class Context {
public:
    // 设置键值对
    void set(const string& key, const Value& value);
    
    // 获取值
    Value get(const string& key) const;
    
    // 检查键是否存在
    bool has(const string& key) const;
    
    // 获取所有键
    vector<string> keys() const;
    
    // 清空上下文
    void clear();
    
    // 获取上下文大小
    size_t size() const;
    
private:
    unordered_map<string, Value> data_;
};
