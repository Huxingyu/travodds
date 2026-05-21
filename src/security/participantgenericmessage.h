#ifndef PARTICIPANT_GENERIC_MESSAGE_H
#define PARTICIPANT_GENERIC_MESSAGE_H

#include "security/securitycommon.h"
#include "rtps/basictypertps.h"
#include <string>

#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

typedef struct MessageIdentity
{
    BuiltinTopicKey_t sourceGuid;
    SequenceNumber_t sequenceNumber;
}MessageIdentity;

typedef struct ParticipantGenericMessage
{
    /* target for the request. Can be GUID_UNKNOWN */
    MessageIdentity messageIdentity;
    MessageIdentity relatedMessageIdentity;
    BuiltinTopicKey_t destinationParticipantKey;
    BuiltinTopicKey_t destinationEndpointKey;
    BuiltinTopicKey_t sourceEndpointKey;
    std::string messageClassId;
    Sequence<DataHolder> messageData;
}ParticipantGenericMessage;

typedef ParticipantGenericMessage ParticipantStatelessMessage;

typedef ParticipantGenericMessage ParticipantVolatileSecureMessage;

TRAVODDS_NAMESPACE_END
#endif // DDSSECURITY
#endif //PARTICIPANT_GENERIC_MESSAGE_H
