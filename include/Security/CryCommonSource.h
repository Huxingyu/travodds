#ifndef CryCommonSource_h__
#define CryCommonSource_h__

#include "travodds_dll.h"
#include "SecurityCommon.h"

TRAVODDS_NAMESPACE_BEGIN

// typedef DDSSecHandle DatawriterCryptoHandle;
// typedef DDSSecHandle DatareaderCryptoHandle;
// typedef DDSSecHandle ParticipantCryptoHandle;

// typedef DDSSecHandle IdentityHandle;
// typedef DDSSecHandle PermissionsHandle;

// typedef DDSSecHandle SharedSecretHandle;


/**
* @typedef	struct SecureSubmessageCategorty_t
*
* @brief   信息类别数据结构.
*/

typedef enum SecureSubmessageCategory_t
{
    DATAWRITER_SUBMESSAGE,
    DATAREADER_SUBMESSAGE,
    INFO_SUBMESSAGE,
    INVALID_SUBMESSAGE
}SecureSubmessageCategory_t;

TRAVODDS_NAMESPACE_END

#endif // CryCommonSource_h__
