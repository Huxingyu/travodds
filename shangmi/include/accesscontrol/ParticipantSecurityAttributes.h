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
#ifndef _SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H_
#define _SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H_

//#include "rtps/common/Property.h"

TRAVODDS_NAMESPACE_BEGIN

typedef unsigned int EndpointSecurityAttributesMask;
typedef unsigned int PluginEndpointSecurityAttributesMask;
#if 0
typedef struct ParticipantSecurityAttributes
{
    /** @brief   是否允许与未验证身份的DomainParticipant匹配 */
    bool allow_unauthenticated_participants;
    /** @brief   两个DomainParticipant匹配是否需要AccessControl插件的授权 */
    bool is_access_protected;
    /** @brief   整个RTPS Message是否需要被CryptoTransform的encode_rtps_message操作进行转换. */
    bool is_rtps_protected;
    /** @brief   发现消息是否需要被保护. */
    bool is_discovery_protected;
    /** @brief   Liveliness消息是否需要被保护. */
    bool is_liveliness_protected;
    /** @brief   为CryptoKeyFactory的register_local_participant操作中的参数participant_properties附加的属性. */
    PropertySeq ac_participant_properties;
    /** @brief The plugin participant attributes */
    PluginParticipantSecurityAttributesMask plugin_participant_attributes;
}ParticipantSecurityAttributes;

typedef struct EndpointSecurityAttributes
{
    /* **从 TopicSecurityAttributes 继承的属性 */
    /** @brief   是否需要使用访问控制插件对本地Reader创建和远端Reader匹配进行检查 */
    bool is_read_protected;
    /** @brief   是否需要使用访问控制插件对本地Writer创建和远端Writer匹配进行检查 */
    bool is_write_protected;
    /** @brief   对实体的发现信息发送时，是使用安全的内建的topics还是使用常规的topics. */
    bool is_discovery_protected;
    /** @brief   Liveliness消息是否需要被保护. */
    bool is_liveliness_protected;
    /* **继承自 TopicSecurityAttributes 的属性结束 */

    /** @brief   是否需要对子消息进行保护. */
    bool is_submessage_protected;
    /** @brief   是否需要对数据负载进行保护. */
    bool is_payload_protected;
    /** @brief   是否需要使用安全插件计算数据实例句柄. */
    bool is_key_protected;
    /** @brief   附加的属性. */
    PropertySeq ac_endpoint_properties;
    /** @brief The plugin endpoint attributes */
    PluginEndpointSecurityAttributesMask plugin_endpoint_attributes;
}EndpointSecurityAttributes;
#endif

typedef uint32_t PluginParticipantSecurityAttributesMask;

#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED                  (0x00000001UL << 0)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED     (0x00000001UL << 1)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED            (0x00000001UL << 2)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED       (0x00000001UL << 3)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED  (0x00000001UL << 4)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED (0x00000001UL << 5)
#define PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID                           (0x00000001UL << 31)
struct PluginParticipantSecurityAttributes
{
    PluginParticipantSecurityAttributes()
        : is_rtps_encrypted(false)
        , is_discovery_encrypted(false)
        , is_liveliness_encrypted(false)
        , is_rtps_origin_authenticated(false)
        , is_discovery_origin_authenticated(false)
        , is_liveliness_origin_authenticated(false)
    {
    }

    explicit PluginParticipantSecurityAttributes(
            const PluginParticipantSecurityAttributesMask mask)
        : is_rtps_encrypted((mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED) != 0)
        , is_discovery_encrypted(
            (mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED) != 0)
        , is_liveliness_encrypted((mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED) != 0)
        , is_rtps_origin_authenticated(
            (mask & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED) != 0)
        , is_discovery_origin_authenticated((mask &
                PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED) != 0)
        , is_liveliness_origin_authenticated((mask &
                PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED) != 0)
    {
    }

    bool is_rtps_encrypted;

    bool is_discovery_encrypted;

    bool is_liveliness_encrypted;

    bool is_rtps_origin_authenticated;

    bool is_discovery_origin_authenticated;

    bool is_liveliness_origin_authenticated;

    inline PluginParticipantSecurityAttributesMask mask() const
    {
        PluginParticipantSecurityAttributesMask rv = PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
        if (is_rtps_encrypted)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED;
        }
        if (is_discovery_encrypted)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
        }
        if (is_liveliness_encrypted)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED;
        }
        if (is_rtps_origin_authenticated)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED;
        }
        if (is_discovery_origin_authenticated)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED;
        }
        if (is_liveliness_origin_authenticated)
        {
            rv |= PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
        }
        return rv;
    }

};

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_ACCESSCONTROL_PARTICIPANTSECURITYATTRIBUTES_H_