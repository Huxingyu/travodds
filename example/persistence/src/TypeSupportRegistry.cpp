#include "TypeSupportRegistry.h"
#include <algorithm>
#include <iostream>

TRAVODDS_NAMESPACE_BEGIN

TypeSupportRegistry& TypeSupportRegistry::instance() {
    static TypeSupportRegistry registry;
    return registry;
}

bool TypeSupportRegistry::register_type_support(const std::string& type_name,
                                               std::shared_ptr<TypeSupport> type_support,
                                               std::function<void*()> sample_factory) {
    if (type_name.empty() || !type_support || !sample_factory) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    // 检查类型是否已存在
    if (type_registry_.find(type_name) != type_registry_.end()) {
        return false; // 类型已存在
    }

    TypeInfo info;
    info.type_support = type_support;
    info.sample_factory = sample_factory;
    info.type_name = type_name;

    type_registry_[type_name] = std::move(info);
    return true;
}

std::shared_ptr<TypeSupport> TypeSupportRegistry::get_type_support(const std::string& type_name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = type_registry_.find(type_name);
    if (it != type_registry_.end()) {
        return it->second.type_support;
    }
    
    return nullptr;
}

void* TypeSupportRegistry::create_sample(const std::string& type_name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = type_registry_.find(type_name);
    if (it != type_registry_.end()) {
        return it->second.sample_factory();
    }
    
    return nullptr;
}

void TypeSupportRegistry::delete_sample(const std::string& type_name, void* sample) const {
    if (sample == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = type_registry_.find(type_name);
    if (it != type_registry_.end()) {
        // 使用TypeSupport的delete_data方法确保正确清理
        it->second.type_support->delete_data(sample);
    } else {
        // 如果找不到TypeSupport，使用通用的delete（作为最后的回退）
        std::cerr << "[TypeSupportRegistry] Warning: TypeSupport not found for type " 
                  << type_name << ", using generic delete" << std::endl;
        delete static_cast<char*>(sample);
    }
}

bool TypeSupportRegistry::is_type_registered(const std::string& type_name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return type_registry_.find(type_name) != type_registry_.end();
}

std::vector<std::string> TypeSupportRegistry::get_registered_types() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<std::string> types;
    types.reserve(type_registry_.size());
    
    for (const auto& pair : type_registry_) {
        types.push_back(pair.first);
    }
    
    std::sort(types.begin(), types.end());
    return types;
}

bool TypeSupportRegistry::unregister_type(const std::string& type_name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = type_registry_.find(type_name);
    if (it != type_registry_.end()) {
        type_registry_.erase(it);
        return true;
    }
    
    return false;
}

void TypeSupportRegistry::clear_all_types() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    type_registry_.clear();
}

TRAVODDS_NAMESPACE_END