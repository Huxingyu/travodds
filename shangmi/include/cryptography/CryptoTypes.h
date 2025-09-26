#ifndef _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_
#define _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_

#include "rtps/common/Token.h"
#include "rtps/security/common/Handle.h"
#include "rtps/security/common/SharedSecretHandle.h"
#include "Security/SecurityException.h"
#include <array>

TRAVODDS_NAMESPACE_BEGIN

typedef std::array<uint8_t, 4> CryptoTransformKind;
typedef std::array<uint8_t, 4> CryptoTransformKeyId;

typedef Token MessageToken;
typedef MessageToken AuthRequestMessageToken;
typedef MessageToken HandshakeMessageToken;

typedef Token CryptoToken;
typedef Token ParticipantCryptoToken;
typedef Token DatawriterCryptoToken;
typedef Token DatareaderCryptoToken;

typedef std::vector<HandshakeMessageToken> HandshakeMessageTokenSeq;
typedef std::vector<CryptoToken> CryptoTokenSeq;
typedef CryptoTokenSeq ParticipantCryptoTokenSeq;
typedef CryptoTokenSeq DatawriterCryptoTokenSeq;
typedef CryptoTokenSeq DatareaderCryptoTokenSeq;

constexpr CryptoTransformKeyId c_transformKeyIdZero = { {0, 0, 0, 0} };

TRAVODDS_NAMESPACE_END
#endif // _SECURITY_CRYPTOGRAPHY_CRYPTOTYPES_H_
