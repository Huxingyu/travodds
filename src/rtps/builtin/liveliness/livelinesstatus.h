
/**
 * @file LivelinesStatus.h
 */
#include <chrono>

#include "type/basictype.h"
#include "rtps/basictypertps.h"
#include "dcps/base/retcode.h"
#include "qos/qos.h"

AGIDDS_NAMESPACE_BEGIN
enum WriterStatus
{
    NOTASSERTED = 0,
    LIVELINESSALIVE = 1,
    NOTALIVE = 2
};

struct LivelinesStatus
{


    LivelinesStatus(GUID_t guidIn, LivelinessQosPolicy livelinessQos): guid_(guidIn), livelinessQos_(livelinessQos), status_(WriterStatus::NOTASSERTED) {}

    LivelinesStatus(): guid_()
    {
        livelinessQos_.kind = LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS;
        livelinessQos_.lease_duration = TIME_INFINITE;
        status_ = WriterStatus::NOTASSERTED;
    }

    ~LivelinesStatus()
    {
    }

    GUID_t guid_;

    LivelinessQosPolicy livelinessQos_;

    unsigned int count_ = 1;

    WriterStatus status_;

    std::chrono::steady_clock::time_point time_;
};

AGIDDS_NAMESPACE_END
