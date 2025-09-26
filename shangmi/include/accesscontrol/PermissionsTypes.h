#ifndef __SECURITY_ACCESSCONTROL_PERMISSIONSTYPES_H__
#define __SECURITY_ACCESSCONTROL_PERMISSIONSTYPES_H__

#include <vector>
#include <string>
#include <cstdint>
#include <ctime>
#include "travodds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

struct Domains
{
    std::vector<std::pair<uint32_t, uint32_t>> ranges;
};

struct Criteria
{
    std::vector<std::string> topics;
    std::vector<std::string> partitions;
};

struct Rule
{
    bool allow;
    Domains domains;
    std::vector<Criteria> publishes;
    std::vector<Criteria> subscribes;
    std::vector<Criteria> relays;
};

struct Validity
{
    std::time_t not_before;
    std::time_t not_after;
};

struct Grant
{
    std::string name;
    std::string subject_name;
    Validity validity;
    std::vector<Rule> rules;
    bool is_default_allow;
};

TRAVODDS_NAMESPACE_END

#endif // __SECURITY_ACCESSCONTROL_PERMISSIONSTYPES_H__