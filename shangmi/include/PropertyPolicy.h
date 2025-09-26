#ifndef PropertyPolicy_h__
#define PropertyPolicy_h__

//#include "rtps/common/Property.h"
#include "rtps/common/BinaryProperty.h"
#include "security/propertyqospolicy.h"

TRAVODDS_NAMESPACE_BEGIN

class PropertyPolicyHelper
{
public:
    /**
     * @brief 根据指定的前缀筛选属性。
     * * @param property_policy 原始的属性集。
     * @param prefix 要匹配的前缀字符串。
     * @return 返回一个新的 PropertyQosPolicy，其中只包含符合前缀的属性，并且属性名中的前缀已被移除。
     */
    static PropertyQosPolicy get_properties_with_prefix(
            const PropertyQosPolicy& property_policy,
            const std::string& prefix);

    // 获取属性的总数 (包括字符串属性和二进制属性)
    static size_t length(
            const PropertyQosPolicy& property_policy);

    // 通过名称查找属性，返回其值的指针 (可修改)
    static std::string* find_property(
            PropertyQosPolicy& property_policy,
            const std::string& name);

    // 通过名称查找属性，返回其值的常量指针 (不可修改)
    static const std::string* find_property(
            const PropertyQosPolicy& property_policy,
            const std::string& name);

    /**
     * @brief 通过名称查找属性。
     * * @param property_policy 要搜索的属性集。
     * @param name 要查找的属性名。
     * @return 如果找到，返回指向整个Property对象(名值对)的指针；否则返回 nullptr。
     */
    static const Property* get_property(
            const PropertyQosPolicy& property_policy,
            const std::string& name);
};

TRAVODDS_NAMESPACE_END

#endif // PropertyPolicy_h__