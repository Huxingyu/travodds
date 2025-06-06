#pragma once
/**
 * @file LivelinessAssert.h
 */
#include <mutex>

#include <common/time/ddstimer.h>
#include "rtps/builtin/liveliness/livelinesstatus.h"

AGIDDS_NAMESPACE_BEGIN

class LivelinessManager;
class LivelinessAssert
{
public:
    LivelinessAssert(LivelinessManager* livelinessManager, bool localWriter = true);

    ~LivelinessAssert();

    LivelinessAssert(const LivelinessAssert& other) = delete;

	/*
	 * 添加writer 管理存活状态  
	 */
    bool AddWriter(LivelinesStatus writer);

    bool RemoveWriter( GUID_t guid);

	/*
	 * 断言某一类writer的存活状态
	 */
    bool AssertLiveliness(GUID_t guid,LivelinessQosPolicyKind kind);

	bool GetWriterLiveLinessKind(GUID_t guid, LivelinessQosPolicyKind& kind);

    bool HasAliveWriters(LivelinessQosPolicyKind kind);

private:

	/*
	 * 断言指定writer的存活状态
	 */
    void AssertWriterLiveliness(LivelinesStatus& writer);

	/*
	 * 查找并更新下一个需要检测存活性的writer
	 */
    bool UpDateCheckedWriter();

	/*
	 * 需要检测存活性的writer时间超期
	 */
    bool TimerExpired(); 


	bool localWriter_;

	std::map<GUID_t,LivelinesStatus> writers_;

    std::mutex mutex_;

	LivelinesStatus* checkedWrtier_;

	std::shared_ptr<Timer> timer_;

    LivelinessManager* livelinessManager_;
};

AGIDDS_NAMESPACE_END
