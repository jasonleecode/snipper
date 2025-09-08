#include "context.h"
#include <vector>

// Context 实现
void Context::set(const string& key, const Value& value) {
    data_[key] = value;
}

Value Context::get(const string& key) const {
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : Value();
}

bool Context::has(const string& key) const {
    return data_.find(key) != data_.end();
}

vector<string> Context::keys() const {
    vector<string> result;
    for (const auto& pair : data_) {
        result.push_back(pair.first);
    }
    return result;
}

void Context::clear() {
    data_.clear();
}

size_t Context::size() const {
    return data_.size();
}
