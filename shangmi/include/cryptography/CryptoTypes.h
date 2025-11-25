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
#ifndef _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_
#define _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_

#include "rtps/basictypertps.h"
#include "security/securitycommon.h"
#include "Handle.h"
#include "SharedSecretHandle.h"
#include "SecurityException.h"
#include <array>

TRAVODDS_NAMESPACE_BEGIN

typedef std::array<uint8_t, 4> CryptoTransformKind;
typedef std::array<uint8_t, 4> CryptoTransformKeyId;

// 使用标准Token类型，不再使用SM前缀
typedef HandshakeMessageToken SMHandshakeMessageToken;
typedef CryptoToken SMCryptoToken;
typedef ParticipantCryptoToken SMParticipantCryptoToken;
typedef DatawriterCryptoToken SMDatawriterCryptoToken;
typedef DatareaderCryptoToken SMDatareaderCryptoToken;

typedef HandshakeMessageTokenSeq SMHandshakeMessageTokenSeq;
typedef CryptoTokenSeq SMCryptoTokenSeq;
typedef ParticipantCryptoTokenSeq SMParticipantCryptoTokenSeq;
typedef DatawriterCryptoTokenSeq SMDatawriterCryptoTokenSeq;
typedef DatareaderCryptoTokenSeq SMDatareaderCryptoTokenSeq;

constexpr CryptoTransformKeyId c_transformKeyIdZero = { {0, 0, 0, 0} };

TRAVODDS_NAMESPACE_END
#endif // _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_
