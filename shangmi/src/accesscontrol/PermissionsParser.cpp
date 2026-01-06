#include "travodds_dll.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "common/log/logger.h"
#include "accesscontrol/PermissionsParser.h"

USING_TRAVODDS_NAMESPACE

#if TIXML2_MAJOR_VERSION >= 6
#define PRINTLINE(node) node->GetLineNum()
#define PRINTLINEPLUSONE(node) node->GetLineNum() + 1
#else
#define PRINTLINE(node) ""
#define PRINTLINEPLUSONE(node) ""
#endif // if TIXML2_MAJOR_VERSION >= 6

static const char* Root_str = "dds";
static const char* Permission_str = "permissions";
static const char* Grant_str = "grant";
static const char* SubjectName_str = "subject_name";
static const char* Validity_str = "validity";
static const char* NotBefore_str = "not_before";
static const char* NotAfter_str = "not_after";
static const char* AllowRule_str = "allow_rule";
static const char* DenyRule_str = "deny_rule";
static const char* Default_str = "default";
static const char* Domains_str = "domains";
static const char* Publish_str = "publish";
static const char* Subscribe_str = "subscribe";
static const char* Relay_str = "relay";
static const char* Topics_str = "topics";
static const char* Topic_str = "topic";
static const char* Partitions_str = "partitions";
static const char* Partition_str = "partition";
static const char* DataTags_str = "data_tags";
static const char* Allow_str = "ALLOW";
static const char* Deny_str = "DENY";
static const char* DomainId_str = "id";
static const char* DomainIdRange_str = "id_range";
static const char* Min_str = "min";
static const char* Max_str = "max";

bool parse_domain_id_set(
    tinyxml2::XMLElement* root,
    Domains& domains);
#if 0
bool parse_domain_id_set(
    tinyxml2::XMLElement* root,
    Domains& domains)
{
    assert(root);

    bool returned_value = false;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        returned_value = true;

        do
        {
            if (strcmp(node->Name(), DomainId_str) == 0)
            {
                uint32_t domain_id = 0;

                if (tinyxml2::XMLError::XML_SUCCESS == node->QueryUnsignedText(&domain_id))
                {
                    domains.ranges.push_back(std::make_pair(domain_id, 0));
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER, "Invalid value of " << DomainId_str <<
                            " tag. Line " << PRINTLINE(node));
                    returned_value = false;
                }
            }
            else if (strcmp(node->Name(), DomainIdRange_str) == 0)
            {
                tinyxml2::XMLElement* subnode = node->FirstChildElement();

                if (subnode != nullptr)
                {
                    uint32_t min_domain_id = 0;

                    if (strcmp(subnode->Name(), Min_str) == 0)
                    {
                        if (tinyxml2::XMLError::XML_SUCCESS != subnode->QueryUnsignedText(&min_domain_id))
                        {
                            REC_LOG_ERROR(XMLPARSER, "Invalid min value of " << DomainId_str <<
                                    " tag. Line " << PRINTLINE(subnode));
                            returned_value = false;
                        }
                        else
                        {
                            uint32_t max_domain_id = 0;

                            if ((subnode = subnode->NextSiblingElement()) != nullptr)
                            {
                                if (strcmp(subnode->Name(), Max_str) == 0)
                                {
                                    if (tinyxml2::XMLError::XML_SUCCESS != subnode->QueryUnsignedText(&max_domain_id))
                                    {
                                        REC_LOG_ERROR(XMLPARSER, "Invalid max value of " << DomainId_str <<
                                                " tag. Line " << PRINTLINE(subnode));
                                        returned_value = false;
                                    }
                                }
                            }

                            if (returned_value)
                            {
                                domains.ranges.push_back(std::make_pair(min_domain_id, max_domain_id));
                            }
                        }
                    }
                    else if (strcmp(subnode->Name(), Max_str) == 0)
                    {
                        uint32_t max_domain_id = 0;
                        if (tinyxml2::XMLError::XML_SUCCESS == subnode->QueryUnsignedText(&max_domain_id))
                        {
                            domains.ranges.push_back(std::make_pair(min_domain_id, max_domain_id));
                        }
                        else
                        {
                            REC_LOG_ERROR(XMLPARSER, "Invalid max value of " << DomainId_str <<
                                    " tag. Line " << PRINTLINE(subnode));
                            returned_value = false;
                        }
                    }
                    else
                    {
                        REC_LOG_ERROR(XMLPARSER, "Expected " << Min_str << " or " << Max_str << " tag. Line " <<
                                PRINTLINE(subnode));
                        returned_value = false;
                    }
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER, "Expected " << Min_str << " and " << Max_str << " tags. Line " <<
                            PRINTLINEPLUSONE(node));
                    returned_value = false;
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Not valid tag. Expected " << DomainId_str << " or " << DomainIdRange_str <<
                        " tag. Line " << PRINTLINE(
                            node));
                returned_value = false;
            }
        }
        while (returned_value && (node = node->NextSiblingElement()) != nullptr);
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Minimum one " << DomainId_str << " or " << DomainIdRange_str << " tag. Line " <<
                PRINTLINEPLUSONE(root));
    }

    return returned_value;
}
#endif

void PermissionsParser::swap(
        PermissionsData& permissions)
{
    permissions = std::move(permissions_);
}

bool PermissionsParser::parse_stream(
        const char* stream,
        size_t stream_length)
{
    assert(stream);

    bool returned_value = false;
    tinyxml2::XMLDocument document;

    if (tinyxml2::XMLError::XML_SUCCESS == document.Parse(stream, stream_length))
    {
        tinyxml2::XMLElement* root = document.RootElement();

        if (root != nullptr)
        {
            if (strcmp(root->Name(), Root_str) == 0)
            {
                tinyxml2::XMLElement* permission_node = root->FirstChildElement();
                if (strcmp(permission_node->Name(), Permission_str) == 0)
                {
                    returned_value = parse_permissions(permission_node);
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER, "Invalid tag. Expected  " << Permission_str << " tag. Line " << PRINTLINE(
                                permission_node));
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Malformed Permissions root. Line " << PRINTLINE(root));
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Not found root node in Permissions XML.");
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Error loading Permissions XML");
    }

    return returned_value;
}

bool PermissionsParser::parse_permissions(
        tinyxml2::XMLElement* root)
{
    assert(root);

    bool returned_value = false;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        returned_value = true;

        do
        {
            if (strcmp(node->Name(), Grant_str) == 0)
            {
                Grant grant;
                if ((returned_value = parse_grant(node, grant)) == true)
                {
                    permissions_.grants.push_back(std::move(grant));
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER,
                        "Invalid tag. Expected  " << Grant_str << " tag. Line " << PRINTLINE(node));
                returned_value = false;
            }
        }
        while (returned_value && (node = node->NextSiblingElement()) != nullptr);
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected at least one " << Grant_str << " tag. Line " << PRINTLINEPLUSONE(root));
    }

    return returned_value;
}

bool PermissionsParser::parse_grant(
        tinyxml2::XMLElement* root,
        Grant& grant)
{
    assert(root);

    const char* name = root->Attribute("name");

    if (name != nullptr)
    {
        grant.name = name;
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER,
                "Attribute name is required in " << Grant_str << " tag. Line " << PRINTLINE(root));
        return false;
    }

    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        if (strcmp(node->Name(), SubjectName_str) == 0)
        {
            const char* text = node->GetText();

            if (text != nullptr)
            {
                grant.subject_name = text;
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER,
                        "Expected text in " << SubjectName_str << " tag. Line " << PRINTLINE(node));
                return false;
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Expected " << SubjectName_str << " tag. Line " << PRINTLINE(node));
            return false;
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected " << SubjectName_str << " tag. Line " << PRINTLINEPLUSONE(root));
        return false;
    }

    tinyxml2::XMLElement* old_node = node;
    (void)old_node;
    node = node->NextSiblingElement();

    if (node != nullptr)
    {
        if (strcmp(node->Name(), Validity_str) == 0)
        {
            if (!parse_validity(node, grant.validity))
            {
                return false;
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Expected " << Validity_str << " tag. Line " << PRINTLINE(node));
            return false;
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected " << Validity_str << " tag. Line " << PRINTLINE(old_node));
        return false;
    }

    old_node = node;
    node = node->NextSiblingElement();

    if (node != nullptr)
    {
        do
        {
            Rule rule;

            if (strcmp(node->Name(), AllowRule_str) == 0)
            {
                rule.allow = true;
            }
            else if (strcmp(node->Name(), DenyRule_str) == 0)
            {
                rule.allow = false;
            }
            else
            {
                break;
            }

            if (!parse_rule(node, rule))
            {
                return false;
            }

            grant.rules.push_back(rule);
            old_node = node;
        }
        while ((node = node->NextSiblingElement()) != nullptr);
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected " << AllowRule_str << " or " << DenyRule_str << " tag. Line " <<
                PRINTLINE(old_node));
        return false;
    }

    if (node != nullptr)
    {
        if (strcmp(node->Name(), Default_str) == 0)
        {
            const char* text = node->GetText();

            if (text != nullptr)
            {
                if (strcmp(text, Allow_str) == 0)
                {
                    grant.is_default_allow = true;
                }
                else if (strcmp(text, Deny_str) == 0)
                {
                    grant.is_default_allow = false;
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER, "Invalid text in" << Default_str << " tag. Line " << PRINTLINE(node));
                    return false;
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected text in" << Default_str << " tag. Line " << PRINTLINE(node));
                return false;
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Invalid tag. Expected tag " << Default_str << ". Line " << PRINTLINE(node));
            return false;
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected tag " << Default_str << ". Line " << PRINTLINE(old_node));
        return false;
    }

    return true;
}

bool PermissionsParser::parse_validity(
        tinyxml2::XMLElement* root,
        Validity&
#if _MSC_VER != 1800
        validity
#endif // if _MSC_VER != 1800
        )
{
    assert(root);

    bool returned_value = false;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        if (strcmp(node->Name(), NotBefore_str) == 0)
        {
            if (node->GetText() != nullptr)
            {
#if _MSC_VER != 1800
                struct tm time;
                memset(&time, 0, sizeof(struct tm));
                std::istringstream stream(node->GetText());
                stream >> std::get_time(&time, "%Y-%m-%dT%T");

                if (!stream.fail())
                {
                    validity.not_before = std::mktime(&time);
#endif // if _MSC_VER != 1800

                tinyxml2::XMLElement* old_node = node;
                (void)old_node;
                node = node->NextSiblingElement();

                if (node != nullptr)
                {
                    if (strcmp(node->Name(), NotAfter_str) == 0)
                    {
#if _MSC_VER != 1800
                        memset(&time, 0, sizeof(struct tm));
                        stream.str(node->GetText());
                        stream.clear();
                        stream >> std::get_time(&time, "%Y-%m-%dT%T");

                        if (!stream.fail())
                        {
                            validity.not_after = std::mktime(&time);
#endif // if _MSC_VER != 1800
                        returned_value = true;
                    }
                    else
                    {
                        REC_LOG_ERROR(XMLPARSER, "Fail parsing datetime value in " << NotAfter_str << " tag. Line " <<
                                PRINTLINE(
                                    node));
                    }
#if _MSC_VER != 1800
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER, "Expected " << NotAfter_str << " tag. Line " << PRINTLINE(node));
                }
#endif // if _MSC_VER != 1800
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER,
                            "Expected " << NotAfter_str << " tag. Line " << PRINTLINEPLUSONE(old_node));
                }
#if _MSC_VER != 1800
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Fail parsing datetime value in " << NotBefore_str << " tag. Line " <<
                        PRINTLINE(node));
            }
#endif // if _MSC_VER != 1800
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected datetime value in " << NotBefore_str << " tag. Line " <<
                        PRINTLINE(node));
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Expected " << NotBefore_str << " tag. Line " << PRINTLINE(node));
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected " << NotBefore_str << " tag. Line " << PRINTLINEPLUSONE(root));
    }

    return returned_value;
}

bool PermissionsParser::parse_rule(
        tinyxml2::XMLElement* root,
        Rule& rule)
{
    assert(root);

    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        if (strcmp(node->Name(), Domains_str) == 0)
        {
            if (!parse_domain_id_set(node, rule.domains))
            {
                return false;
            }
        }
        else
        {
            REC_LOG_ERROR(XMLPARSER, "Expected " << Domains_str << " tag. Line " << PRINTLINE(node));
            return false;
        }
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected " << Domains_str << " tag. Line " << PRINTLINEPLUSONE(root));
        return false;
    }

    node = node->NextSiblingElement();

    if (node != nullptr)
    {
        do
        {
            Criteria criteria;

            if (strcmp(node->Name(), Publish_str) == 0)
            {
                if (!parse_criteria(node, criteria))
                {
                    return false;
                }

                rule.publishes.push_back(std::move(criteria));
            }
            else if (strcmp(node->Name(), Subscribe_str) == 0)
            {
                if (!parse_criteria(node, criteria))
                {
                    return false;
                }

                rule.subscribes.push_back(std::move(criteria));
            }
            else if (strcmp(node->Name(), Relay_str) == 0)
            {

                if (!parse_criteria(node, criteria))
                {
                    return false;
                }

                rule.relays.push_back(std::move(criteria));
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected " << Publish_str << " or " << Subscribe_str <<
                        " or " << Relay_str << " tag. Line " << PRINTLINE(node));
                return false;
            }
        }
        while ((node = node->NextSiblingElement()) != nullptr);
    }

    return true;
}

bool PermissionsParser::parse_criteria(
        tinyxml2::XMLElement* root,
        Criteria& criteria)
{
    bool returned_value = true;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        do
        {
            if (strcmp(node->Name(), Topics_str) == 0)
            {
                returned_value = parse_topic(node, criteria.topics);
            }
            else if (strcmp(node->Name(), Partitions_str) == 0)
            {
                returned_value = parse_partition(node, criteria.partitions);
            }
            else if (strcmp(node->Name(), DataTags_str) == 0)
            {
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected " << Topics_str << " or " << Partitions_str <<
                        " or " << DataTags_str << " tag. Line " << PRINTLINE(node));
                returned_value = false;
            }
        }
        while (returned_value && (node = node->NextSiblingElement()) != nullptr);
    }

    if (returned_value && criteria.partitions.empty())
    {
        criteria.partitions.push_back(std::string());
    }

    return returned_value;
}

bool PermissionsParser::parse_topic(
        tinyxml2::XMLElement* root,
        std::vector<std::string>& topics)
{
    bool returned_value = false;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        returned_value = true;

        do
        {
            if (strcmp(node->Name(), Topic_str) == 0)
            {
                if (node->GetText() != nullptr)
                {
                    std::string topic = node->GetText();
                    topics.push_back(std::move(topic));
                }
                else
                {
                    REC_LOG_ERROR(XMLPARSER,
                            "Expected topic name in " << Topic_str << " tag. Line " << PRINTLINE(node));
                    returned_value = false;
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected " << Topic_str << " tag. Line " << PRINTLINE(node));
                returned_value = false;
            }
        }
        while (returned_value && (node = node->NextSiblingElement()) != nullptr);
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER, "Expected at least one " << Topic_str << " tag. Line " << PRINTLINEPLUSONE(root));
    }

    return returned_value;
}

bool PermissionsParser::parse_partition(
        tinyxml2::XMLElement* root,
        std::vector<std::string>& partitions)
{
    bool returned_value = false;
    tinyxml2::XMLElement* node = root->FirstChildElement();

    if (node != nullptr)
    {
        returned_value = true;

        do
        {
            if (strcmp(node->Name(), Partition_str) == 0)
            {
                if (node->GetText() != nullptr)
                {
                    std::string partition = node->GetText();
                    partitions.push_back(std::move(partition));
                }
                else
                {
                    // Detect empty partition tag
                    if (node->NoChildren())
                    {
                        partitions.push_back(std::string());
                    }
                    else
                    {
                        REC_LOG_ERROR(XMLPARSER, "Expected topic name in " << Partition_str << " tag. Line " << PRINTLINE(
                                    node));
                        returned_value = false;
                    }
                }
            }
            else
            {
                REC_LOG_ERROR(XMLPARSER, "Expected " << Partition_str << " tag. Line " << PRINTLINE(node));
                returned_value = false;
            }
        }
        while (returned_value && (node = node->NextSiblingElement()) != nullptr);
    }
    else
    {
        REC_LOG_ERROR(XMLPARSER,
                "Expected at least one " << Partition_str << " tag. Line " << PRINTLINEPLUSONE(root));
    }

    return returned_value;
}