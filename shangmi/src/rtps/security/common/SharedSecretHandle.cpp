#include "travodds_dll.h"
#include "SharedSecretHandle.h"

USING_TRAVODDS_NAMESPACE

const char* const SMSharedSecret::class_id_ = "SMSharedSecret";

std::vector<uint8_t>* SMSharedSecretHelper::find_data_value(SMSharedSecret& sharedsecret, const std::string& name)
{
    std::vector<uint8_t>* returnedValue = nullptr;

    for (auto property = sharedsecret.data_.begin(); property != sharedsecret.data_.end(); ++property)
    {
        if (property->name().compare(name) == 0)
        {
            returnedValue = &property->value();
            break;
        }
    }

    return returnedValue;
}

const std::vector<uint8_t>* SMSharedSecretHelper::find_data_value(const SMSharedSecret& sharedsecret, const std::string& name)
{
    const std::vector<uint8_t>* returnedValue = nullptr;

    for (auto property = sharedsecret.data_.begin(); property != sharedsecret.data_.end(); ++property)
    {
        if (property->name().compare(name) == 0)
        {
            returnedValue = &property->value();
            break;
        }
    }

    return returnedValue;
}

const std::vector<uint8_t>* SMSharedSecretHelper::find_data_value(
        const SMSecretHandle& secret,
        const std::string& name)
{
    const std::vector<uint8_t>* returnedValue = nullptr;
    const SMSharedSecretHandle& sh = SMSharedSecretHandle::narrow(secret);

    if (sh.nil())
    {
        return nullptr;
    }

    const SMSharedSecret& sharedsecret = **sh;

    for (auto property = sharedsecret.data_.begin(); property != sharedsecret.data_.end(); ++property)
    {
        if (property->name().compare(name) == 0)
        {
            returnedValue = &property->value();
            break;
        }
    }

    return returnedValue;
}
