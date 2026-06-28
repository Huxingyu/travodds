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
#ifndef CRYCOMMON_SOURCE_H
#define CRYCOMMON_SOURCE_H

#include "security/securitycommon.h"

#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

typedef DDSSecHandle DatawriterCryptoHandle;
typedef DDSSecHandle DatareaderCryptoHandle;
typedef DDSSecHandle ParticipantCryptoHandle;
typedef DDSSecHandle IdentityHandle;
typedef DDSSecHandle PermissionsHandle;
typedef DDSSecHandle SharedSecretHandle;

/**
 * @typedef	struct DDS_SecureSubmessageCategorty_t
 *
 * @brief   信息类别数据结构.
 */

enum SecureSubmessageCategory_t
{
    DATAWRITER_SUBMESSAGE,
    DATAREADER_SUBMESSAGE,
    INFO_SUBMESSAGE,
    INVALID_SUBMESSAGE
};

TRAVODDS_NAMESPACE_END
#endif //DDSSECURITY
#endif // CRYCOMMON_SOURCE_H
