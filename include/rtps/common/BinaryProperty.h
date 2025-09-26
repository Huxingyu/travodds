#ifndef BINARYPROPERTY_H
#define BINARYPROPERTY_H
#include<string>
#include<vector>
#include "travodds_dll.h"
using std::string;
using std::vector;

TRAVODDS_NAMESPACE_BEGIN
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

class BinaryPropertyHelper
{
public:

    static size_t serialized_size(
            const BinaryProperty& binary_property,
            size_t current_alignment = 0)
    {
        if (binary_property.propagate())
        {
            size_t initial_alignment = current_alignment;

            current_alignment += 4 + alignment(current_alignment, 4) + binary_property.name().size() + 1;
            current_alignment += 4 + alignment(current_alignment, 4) + binary_property.value().size();

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

TRAVODDS_NAMESPACE_END

#endif // BINARYPROPERTY_H
