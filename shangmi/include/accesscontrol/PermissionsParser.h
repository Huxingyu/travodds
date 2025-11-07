//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef __SECURITY_ACCESSCONTROL_PERMISSIONSPARSER_H__
#define __SECURITY_ACCESSCONTROL_PERMISSIONSPARSER_H__

#include <tinyxml2.h>
#include "accesscontrol/PermissionsTypes.h"

TRAVODDS_NAMESPACE_BEGIN

struct PermissionsData
{
    std::vector<Grant> grants;
};

class PermissionsParser
{
public:

    bool parse_stream(
            const char* stream,
            size_t stream_length);

    void swap(
            PermissionsData& permissions);

private:

    bool parse_permissions(
            tinyxml2::XMLElement* root);

    bool parse_grant(
            tinyxml2::XMLElement* root,
            Grant& grant);

    bool parse_validity(
            tinyxml2::XMLElement* root,
            Validity& validity);

    bool parse_rule(
            tinyxml2::XMLElement* root,
            Rule& rule);

    bool parse_criteria(
            tinyxml2::XMLElement* root,
            Criteria& criteria);

    bool parse_topic(
            tinyxml2::XMLElement* root,
            std::vector<std::string>& topics);

    bool parse_partition(
            tinyxml2::XMLElement* root,
            std::vector<std::string>& partitions);

    PermissionsData permissions_;
};

TRAVODDS_NAMESPACE_END
#endif // __SECURITY_ACCESSCONTROL_PERMISSIONSPARSER_H__