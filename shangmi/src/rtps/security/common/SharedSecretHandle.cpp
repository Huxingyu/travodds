#include "travodds_dll.h"
#include "rtps/security/common/SharedSecretHandle.h"

USING_TRAVODDS_NAMESPACE

const char* const SharedSecret::class_id_ = "SharedSecret";

std::vector<uint8_t>* SharedSecretHelper::find_data_value(SharedSecret& sharedsecret, const std::string& name)
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

const std::vector<uint8_t>* SharedSecretHelper::find_data_value(const SharedSecret& sharedsecret, const std::string& name)
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

const std::vector<uint8_t>* SharedSecretHelper::find_data_value(
        const SecretHandle& secret,
        const std::string& name)
{
    const std::vector<uint8_t>* returnedValue = nullptr;
    const SharedSecretHandle& sh = SharedSecretHandle::narrow(secret);

    if (sh.nil())
    {
        return nullptr;
    }

    const SharedSecret& sharedsecret = **sh;

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
