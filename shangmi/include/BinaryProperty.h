#ifndef BINARYPROPERTY_H
#define BINARYPROPERTY_H
#include<string>
#include<vector>
#include "travodds_dll.h"
#include "type/basictype.h"
#include "security/securitycommon.h"
using std::string;
using std::vector;

TRAVODDS_NAMESPACE_BEGIN
#if 0
class BinaryProperty
{
public:

    BinaryProperty() :m_propagate(false) {}

    BinaryProperty(const BinaryProperty& property) :m_name(property.m_name),
        m_value(property.m_value), m_propagate(property.m_propagate)
    {
    }

    BinaryProperty(const string& name, const vector<uint8_t>& value) :m_name(name), m_value(value)
    {
    }

    BinaryProperty& operator=(const BinaryProperty& property)
    {
        this->m_name = property.m_name;
        this->m_value = property.m_value;
        this->m_propagate = property.m_propagate;
        return *this;
    }

    bool operator==(const BinaryProperty& right) const
    {
        return (this->m_name == right.m_name) && (this->m_value == right.m_value);
    }

    void name(const string& name)
    {
        this->m_name = name;
    }


    void value(const vector<uint8_t>& value)
    {
        this->m_value = value;
    }

    void propagate(bool propagate)
    {
        this->m_propagate = propagate;
    }

    const string& name() const
    {
        return m_name;
    }

    string& name()
    {
        return m_name;
    }

    const vector<uint8_t>& value() const
    {
        return m_value;
    }
    vector<uint8_t>& value()
    {
        return m_value;
    }

    bool propagate() const
    {
        return m_propagate;
    }
    bool& propagate(){
        return m_propagate;
    }
private:
    string m_name;
    vector<uint8_t> m_value;
    bool m_propagate;
};

typedef vector<BinaryProperty> BinaryPropertySeq;
#endif


class BinaryPropertyHelper
{
public:

    static size_t serialized_size(
            const BinaryProperty& binary_property,
            size_t current_alignment = 0)
    {
        if (binary_property.propagate)
        {
            size_t initial_alignment = current_alignment;

            current_alignment += 4 + alignment(current_alignment, 4) + binary_property.name.size() + 1;
            current_alignment += 4 + alignment(current_alignment, 4) + binary_property.value.size();

            return current_alignment - initial_alignment;
        }
        else
        {
            return 0;
        }
    }

    static size_t serialized_size(
            const BinaryPropertySeq& binary_properties,
            size_t current_alignment = 0)
    {
        size_t initial_alignment = current_alignment;

        current_alignment += 4 + alignment(current_alignment, 4);
        for (auto binary_property = binary_properties.begin(); binary_property != binary_properties.end();
                ++binary_property)
        {
            current_alignment += serialized_size(*binary_property, current_alignment);
        }

        return current_alignment - initial_alignment;
    }

private:

    inline static size_t alignment(
            size_t current_alignment,
            size_t dataSize)
    {
        return (dataSize - (current_alignment % dataSize)) & (dataSize - 1);
    }

};

class PropertyHelper
{
public:
    /**
     * @brief 通过名称查找属性，返回指向Property对象的指针（可修改）
     * @param properties 要搜索的属性序列
     * @param name 要查找的属性名
     * @return 如果找到，返回指向Property对象的指针；否则返回 nullptr
     */
    static Property* find_property(
            PropertySeq& properties,
            const std::string& name)
    {
        for (auto it = properties.begin(); it != properties.end(); ++it)
        {
            if (it->name.compare(name) == 0)
            {
                return &(*it);
            }
        }
        return nullptr;
    }

    /**
     * @brief 通过名称查找属性，返回指向Property对象的常量指针（只读）
     * @param properties 要搜索的属性序列
     * @param name 要查找的属性名
     * @return 如果找到，返回指向Property对象的常量指针；否则返回 nullptr
     */
    static const Property* find_property(
            const PropertySeq& properties,
            const std::string& name)
    {
        for (auto it = properties.begin(); it != properties.end(); ++it)
        {
            if (it->name.compare(name) == 0)
            {
                return &(*it);
            }
        }
        return nullptr;
    }

    /**
     * @brief 通过名称查找属性值，返回指向属性值的指针（可修改）
     * @param properties 要搜索的属性序列
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的指针；否则返回 nullptr
     */
    static std::string* find_property_value(
            PropertySeq& properties,
            const std::string& name)
    {
        Property* prop = find_property(properties, name);
        return prop != nullptr ? &prop->value : nullptr;
    }

    /**
     * @brief 通过名称查找属性值，返回指向属性值的常量指针（只读）
     * @param properties 要搜索的属性序列
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的常量指针；否则返回 nullptr
     */
    static const std::string* find_property_value(
            const PropertySeq& properties,
            const std::string& name)
    {
        const Property* prop = find_property(properties, name);
        return prop != nullptr ? &prop->value : nullptr;
    }
};

class DataHolderHelper
{
public:
    /**
     * @brief 在DataHolder中查找BinaryProperty，返回指向BinaryProperty对象的指针（可修改）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向BinaryProperty对象的指针；否则返回 nullptr
     */
    static BinaryProperty_t* find_binary_property(
            DataHolder& token,
            const std::string& name)
    {
        for (auto it = token.binary_properties.begin(); it != token.binary_properties.end(); ++it)
        {
            if (it->name.compare(name) == 0)
            {
                return &(*it);
            }
        }
        return nullptr;
    }

    /**
     * @brief 在DataHolder中查找BinaryProperty，返回指向BinaryProperty对象的常量指针（只读）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向BinaryProperty对象的常量指针；否则返回 nullptr
     */
    static const BinaryProperty_t* find_binary_property(
            const DataHolder& token,
            const std::string& name)
    {
        for (auto it = token.binary_properties.begin(); it != token.binary_properties.end(); ++it)
        {
            if (it->name.compare(name) == 0)
            {
                return &(*it);
            }
        }
        return nullptr;
    }

    /**
     * @brief 在DataHolder中查找BinaryProperty的值，返回指向属性值的指针（可修改）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的指针；否则返回 nullptr
     */
    static std::vector<uint8_t>* find_binary_property_value(
            DataHolder& token,
            const std::string& name)
    {
        BinaryProperty_t* prop = find_binary_property(token, name);
        return prop != nullptr ? &prop->value : nullptr;
    }

    /**
     * @brief 在DataHolder中查找BinaryProperty的值，返回指向属性值的常量指针（只读）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的常量指针；否则返回 nullptr
     */
    static const std::vector<uint8_t>* find_binary_property_value(
            const DataHolder& token,
            const std::string& name)
    {
        const BinaryProperty_t* prop = find_binary_property(token, name);
        return prop != nullptr ? &prop->value : nullptr;
    }

    /**
     * @brief 在DataHolder中查找Property的值，返回指向属性值的指针（可修改）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的指针；否则返回 nullptr
     */
    static std::string* find_property_value(
            DataHolder& token,
            const std::string& name)
    {
        return PropertyHelper::find_property_value(token.properties, name);
    }

    /**
     * @brief 在DataHolder中查找Property的值，返回指向属性值的常量指针（只读）
     * @param token 要搜索的DataHolder对象
     * @param name 要查找的属性名
     * @return 如果找到，返回指向属性值的常量指针；否则返回 nullptr
     */
    static const std::string* find_property_value(
            const DataHolder& token,
            const std::string& name)
    {
        return PropertyHelper::find_property_value(token.properties, name);
    }
};

TRAVODDS_NAMESPACE_END

#endif // BINARYPROPERTY_H
