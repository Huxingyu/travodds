
/**
 * @file LivelinesStatus.h
 */
#include <chrono>

#include "type/basictype.h"
#include "rtps/basictypertps.h"
#include "dcps/base/retcode.h"
#include "qos/qos.h"

TRAVODDS_NAMESPACE_BEGIN
enum WriterStatus
{
    NOTASSERTED = 0,
    LIVELINESSALIVE = 1,
    NOTALIVE = 2
};

/**
 * @brief 保存writer相关存活信息的结构
 */
struct LivelinesStatus
{
    /**
     * @brief 构造函数。
     *
     * @param [in] guidIn writer guid。
     * @param [in] livelinessQos liveliness Qos配置信息。
     */
    LivelinesStatus(GUID_t guidIn, LivelinessQosPolicy livelinessQos): guid_(guidIn), livelinessQos_(livelinessQos), status_(WriterStatus::NOTASSERTED) {}

    /**
     * @brief 构造函数，生成默认值LivelinesStatus对象。
     */
    LivelinesStatus(): guid_()
    {
        livelinessQos_.kind = LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS;
        livelinessQos_.lease_duration = TIME_INFINITE;
        status_ = WriterStatus::NOTASSERTED;
    }

    /**
     * @brief 析构函数。
     */
    ~LivelinesStatus()
    {
    }

    GUID_t guid_;

    LivelinessQosPolicy livelinessQos_;

    unsigned int count_ = 1;

    WriterStatus status_;

    std::chrono::steady_clock::time_point time_;
};

TRAVODDS_NAMESPACE_END
