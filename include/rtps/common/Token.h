#ifndef TOKEN_H
#define TOKEN_H
#include "BinaryProperty.h"
//#include "Property.h"

TRAVODDS_NAMESPACE_BEGIN
class DataHolder
{
public:
    DataHolder() {}

    DataHolder(const DataHolder& data_holder) :
        class_id(data_holder.class_id),
        m_properties(data_holder.m_properties),
        m_binary_properties(data_holder.m_binary_properties)
    {
    }

    DataHolder& operator=(const DataHolder& data_holder)
    {
        this->class_id = data_holder.class_id;
        this->m_properties = data_holder.m_properties;
        this->m_binary_properties = data_holder.m_binary_properties;
        return *this;
    }

    // 检查 Token 对象是否是空的
    bool is_nil() const
    {
        return this->class_id.empty();
    }

    void SetClassID(const string& classid)
    {
        this->class_id = classid;
    }

    string& GetClassID()
    {
        return this->class_id;
    }

    const string& GetClassID() const
    {
        return this->class_id;
    }

    PropertySeq& GetProperties()
    {
        return this->m_properties;
    }

    const PropertySeq& GetProperties() const
    {
        return this->m_properties;
    }


    BinaryPropertySeq& GetBinaryProperties()
    {
        return this->m_binary_properties;
    }

    const BinaryPropertySeq& GetBinaryProperties() const
    {
        return this->m_binary_properties;
    }
    string* FindPropertyValue(const string& name)
    {
        string* returnedValue = nullptr;
        for (auto property = m_properties.begin(); property != m_properties.end(); ++property)
        {
            if (property->name.compare(name) == 0)
            {
                returnedValue = &property->value;
                break;
            }
        }
        return returnedValue;
    }
    const string* FindPropertyValue(const string& name) const
    {
        const string* returnedValue = nullptr;
        for (auto property = m_properties.begin(); property != m_properties.end(); ++property)
        {
            if (property->name.compare(name) == 0)
            {
                returnedValue = &property->value;
                break;
            }
        }
        return returnedValue;
    }

    vector<uint8_t>* FindBinaryPropertyValue(const string& name)
    {
        vector<uint8_t>* returnedValue = nullptr;
        for (auto property = m_binary_properties.begin(); property != m_binary_properties.end(); ++property)
        {
            if (property->name().compare(name) == 0)
            {
                returnedValue = &(property->value());
                break;
            }
        }
        return returnedValue;
    }

    BinaryProperty* FindBinaryProperty(const std::string& name)
    {
        BinaryProperty* returnedValue = nullptr;

        for (auto property = m_binary_properties.begin(); property != m_binary_properties.end(); ++property)
        {
            if (property->name().compare(name) == 0)
            {
                returnedValue = &(*property);
                break;
            }
        }

        return returnedValue;
    }

    Property* FindProperty(const string& name)
    {
        Property* returnValue = nullptr;

        for (auto property = m_properties.begin(); property != m_properties.end(); ++property)
        {
            if (property->name.compare(name) == 0)
            {
                returnValue = &(*property);
                break;
            }
        }
        return returnValue;
    }

    const Property* FindProperty(const string& name) const
    {
        const Property* returnValue = nullptr;

        for (auto property = m_properties.begin(); property != m_properties.end(); ++property)
        {
            if (property->name.compare(name) == 0)
            {
                returnValue = &(*property);
                break;
            }
        }
        return returnValue;
    }



private:
    string class_id;
    PropertySeq m_properties;
    BinaryPropertySeq m_binary_properties;
};

typedef std::vector<DataHolder> DataHolderSeq;
typedef DataHolder Token;
typedef Token IdentityToken;

typedef Token IdentityStatusToken;
typedef Token PermissionsToken;
typedef Token AuthenticatedPeerCredentialToken;
typedef Token PermissionsCredentialToken;

typedef Token HandshakeMessageToken;

TRAVODDS_NAMESPACE_END

#endif // TOKEN_H