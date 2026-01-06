#include "travodds_dll.h"
#include "qos/qos.h"
#include "PropertyPolicy.h"
#include <algorithm> 

USING_TRAVODDS_NAMESPACE
    /**
 * @brief 从一个 PropertyQosPolicy 中提取所有具有指定前缀的属性。
 * * 遍历 PropertyQosPolicy 中的所有 properties 和 binary_properties。
 * 如果一个属性的名称以指定前缀开头，则复制该属性，
 * 从其名称中移除前缀，并将其添加到新的 PropertyQosPolicy 对象中。
 *
 * @param property_policy 要在其中搜索的源 PropertyQosPolicy。
 * @param prefix 要匹配的属性名前缀。
 * @return 一个新的 PropertyQosPolicy 对象，包含所有匹配且已移除前缀的属性。
 */
PropertyQosPolicy PropertyPolicyHelper::get_properties_with_prefix(
    const PropertyQosPolicy& property_policy,
    const std::string& prefix)
{
PropertyQosPolicy returned_property_policy;

// 搜索 properties
std::for_each(property_policy.properties().begin(), property_policy.properties().end(),
        [&returned_property_policy, &prefix](const Property& property)
        {
            if (property.name.compare(0, prefix.size(), prefix) == 0)
            {
                Property new_property(property);
                new_property.name.erase(0, prefix.size());
                returned_property_policy.properties().emplace_back(new_property);
            }
        });

// 搜索 binary_properties
std::for_each(property_policy.binary_properties().begin(), property_policy.binary_properties().end(),
        [&returned_property_policy, &prefix](const BinaryProperty& property)
        {
            if (property.name.compare(0, prefix.size(), prefix) == 0)
            {
                BinaryProperty new_property(property);
                new_property.name.erase(0, prefix.size());
                returned_property_policy.binary_properties().emplace_back(new_property);
            }
        });

return returned_property_policy;
}

/**
* @brief 计算 PropertyQosPolicy 中属性的总数。
*
* @param property_policy 要检查的 PropertyQosPolicy 对象。
* @return properties 和 binary_properties 的总和。
*/
size_t PropertyPolicyHelper::length(
    const PropertyQosPolicy& property_policy)
{
return property_policy.properties().size() +
       property_policy.binary_properties().size();
}

/**
* @brief 按名称查找属性，并返回指向其值的可修改指针。
*
* @param property_policy 要搜索的 PropertyQosPolicy 对象。
* @param name 要查找的属性的名称。
* @return 如果找到，则返回指向属性值的指针；否则返回 nullptr。
*/
std::string* PropertyPolicyHelper::find_property(
    PropertyQosPolicy& property_policy,
    const std::string& name)
{
std::string* returnedValue = nullptr;

for (auto property = property_policy.properties().begin(); property != property_policy.properties().end();
        ++property)
{
    if (property->name.compare(name) == 0)
    {
        returnedValue = &property->value;
        break;
    }
}

return returnedValue;
}

/**
* @brief 按名称查找属性，并返回指向其值的常量指针 (const 重载)。
*
* @param property_policy 要搜索的 PropertyQosPolicy 对象。
* @param name 要查找的属性的名称。
* @return 如果找到，则返回指向属性值的常量指针；否则返回 nullptr。
*/
const std::string* PropertyPolicyHelper::find_property(
    const PropertyQosPolicy& property_policy,
    const std::string& name)
{
const std::string* returnedValue = nullptr;

for (auto property = property_policy.properties().begin(); property != property_policy.properties().end();
        ++property)
{
    if (property->name.compare(name) == 0)
    {
        returnedValue = &property->value;
        break;
    }
}

return returnedValue;
}

/**
* @brief 按名称获取对整个 Property 对象的常量引用。
*
* @param property_policy 要搜索的 PropertyQosPolicy 对象。
* @param name 要查找的属性的名称。
* @return 如果找到，则返回指向 Property 对象的常量指针；否则返回 nullptr。
*/
const Property* PropertyPolicyHelper::get_property(
    const PropertyQosPolicy& property_policy,
    const std::string& name)
{
const Property* returnedValue = nullptr;

for (auto property = property_policy.properties().begin(); property != property_policy.properties().end();
        ++property)
{
    if (property->name.compare(name) == 0)
    {
        returnedValue = &*property;
        break;
    }
}

return returnedValue;
}

