#pragma once

#include "../../../include/travodds_dll.h"
#include "dcps/topic/typesupport.h"
#include "dcps/domain/domainparticipant.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief 类型支持注册表，管理所有注册的数据类型及其TypeSupport
 * 
 * 这个类提供了一个集中的注册机制，允许持久化服务处理任意DDS数据类型
 * 而不仅仅是硬编码的HelloWorld类型。
 */
class DDS_DLL TypeSupportRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static TypeSupportRegistry& instance();

    /**
     * @brief 注册一个数据类型的TypeSupport
     * 
     * @param type_name 数据类型名称（如 "HelloWorld"）
     * @param type_support TypeSupport实例的共享指针
     * @param sample_factory 创建该类型样本的工厂函数
     * @return true 注册成功
     * @return false 注册失败（通常是类型名已存在）
     */
    bool register_type_support(const std::string& type_name,
                              std::shared_ptr<TypeSupport> type_support,
                              std::function<void*()> sample_factory);

    /**
     * @brief 获取指定类型的TypeSupport
     * 
     * @param type_name 数据类型名称
     * @return std::shared_ptr<TypeSupport> TypeSupport实例，如果类型未注册则返回nullptr
     */
    std::shared_ptr<TypeSupport> get_type_support(const std::string& type_name) const;

    /**
     * @brief 创建指定类型的样本实例
     * 
     * @param type_name 数据类型名称
     * @return void* 新创建的样本指针，调用者负责释放；如果类型未注册则返回nullptr
     */
    void* create_sample(const std::string& type_name) const;

    /**
     * @brief 删除指定类型的样本实例
     * 
     * @param type_name 数据类型名称
     * @param sample 要删除的样本指针
     */
    void delete_sample(const std::string& type_name, void* sample) const;

    /**
     * @brief 检查指定类型是否已注册
     * 
     * @param type_name 数据类型名称
     * @return true 类型已注册
     * @return false 类型未注册
     */
    bool is_type_registered(const std::string& type_name) const;

    /**
     * @brief 获取所有已注册的类型名称
     * 
     * @return std::vector<std::string> 已注册的类型名称列表
     */
    std::vector<std::string> get_registered_types() const;

    /**
     * @brief 注销指定类型
     * 
     * @param type_name 要注销的数据类型名称
     * @return true 注销成功
     * @return false 注销失败（类型不存在）
     */
    bool unregister_type(const std::string& type_name);

    /**
     * @brief 清空所有注册的类型
     */
    void clear_all_types();

private:
    TypeSupportRegistry() = default;
    ~TypeSupportRegistry() = default;
    
    // 禁止复制和移动
    TypeSupportRegistry(const TypeSupportRegistry&) = delete;
    TypeSupportRegistry& operator=(const TypeSupportRegistry&) = delete;
    TypeSupportRegistry(TypeSupportRegistry&&) = delete;
    TypeSupportRegistry& operator=(TypeSupportRegistry&&) = delete;

    /**
     * @brief 注册的类型信息
     */
    struct TypeInfo {
        std::shared_ptr<TypeSupport> type_support;
        std::function<void*()> sample_factory;
        std::string type_name;
    };

    mutable std::mutex registry_mutex_;
    std::map<std::string, TypeInfo> type_registry_;
};

/**
 * @brief 便利的模板函数，用于注册具体的数据类型
 * 
 * @tparam T 数据类型
 * @tparam TypeSupportT 对应的TypeSupport类型
 * @param type_name 类型名称
 * @param type_support_factory 创建TypeSupport的工厂函数
 * @return true 注册成功
 * @return false 注册失败
 */
template<typename T, typename TypeSupportT>
bool register_data_type(const std::string& type_name, 
                       std::function<std::shared_ptr<TypeSupportT>()> type_support_factory = nullptr) {
    auto& registry = TypeSupportRegistry::instance();
    
    std::shared_ptr<TypeSupport> type_support;
    if (type_support_factory) {
        type_support = type_support_factory();
    } else {
        // 使用默认的get_instance方法，但不拥有所有权（单例）
        TypeSupportT* instance = TypeSupportT::get_instance();
        if (instance) {
            type_support = std::shared_ptr<TypeSupport>(instance, [](TypeSupport*){
                // 不删除单例实例
            });
        }
    }
    
    if (!type_support) {
        return false;
    }
    
    // 样本工厂函数 - 使用TypeSupport的create_data方法确保一致性
    auto sample_factory = [type_support]() -> void* {
        return type_support->create_data();
    };
    
    return registry.register_type_support(type_name, type_support, sample_factory);
}

/**
 * @brief 统一注册数据类型到DomainParticipant和TypeSupportRegistry
 * 
 * 这个函数同时处理DDS框架层面的类型注册和持久化服务层面的类型注册，
 * 确保类型在两个系统中都可用。
 * 
 * @tparam T 数据类型
 * @tparam TypeSupportT TypeSupport类型
 * @param participant DomainParticipant实例
 * @param type_name 类型名称
 * @return true 注册成功
 * @return false 注册失败
 */
template<typename T, typename TypeSupportT>
bool register_type_unified(DomainParticipant* participant, const std::string& type_name) {
    if (!participant) {
        return false;
    }
    
    // 1. 注册到DomainParticipant (DDS框架需要)
    ReturnCode_t dds_ret = participant->registe_type(type_name, TypeSupportT::get_instance());
    if (dds_ret != RETCODE_OK) {
        return false;
    }
    
    // 2. 注册到TypeSupportRegistry (持久化服务需要)
    bool registry_ret = register_data_type<T, TypeSupportT>(type_name);
    if (!registry_ret) {
        return false;
    }
    
    return true;
}

TRAVODDS_NAMESPACE_END