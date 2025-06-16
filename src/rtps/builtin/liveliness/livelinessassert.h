#pragma once
/**
 * @file LivelinessAssert.h
 */
#include <mutex>

#include <common/time/ddstimer.h>
#include "rtps/builtin/liveliness/livelinesstatus.h"

AGIDDS_NAMESPACE_BEGIN

class LivelinessManager;

/**
 * @brief LivelinessAssert类, 管理一组数据写者的活性的类。
 * 数据写者由其LivelinesStatus详细信息表示使用共享的定时事件，并将活性更改通知外部类。
 */
class LivelinessAssert
{
public:
	/**
	 * @brief 构造函数。
	 * 
	 * @param [in] livelinessManager liveliness管理类指针。
	 * @param [in] localWriter 是否为本地weiter标志（默认：true）。
	 */
    LivelinessAssert(LivelinessManager* livelinessManager, bool localWriter = true);

	/**
	 * @brief 析构函数。
	 */
    ~LivelinessAssert();

	 /**
	  * @brief 添加writer信息用于存活状态管理集合中。
	  *
	  * @param [in] writer writer信息。
	  *
	  * @return 添加成功返回true，否则返回false。
	  */
    bool AddWriter(LivelinesStatus writer);

	/**
	 * @brief 从存活状态管理集合中删除writer信息。
	 *
	 * @param [in] guid 要删除writer信息的guid。
	 *
	 * @return 删除成功返回true，否则返回false。
	 */
    bool RemoveWriter( GUID_t guid);

	 /**
	  * @brief 根据guid更新对应的存活状态管理集合中writer的存活状态。
	  *
	  * @param [in] guid 要更新状态writer的guid。
	  * @param [in] kind Liveliness Qos信息。
	  *
	  * @return 更新成功返回true，否则返回false。
	  */
    bool AssertLiveliness(GUID_t guid,LivelinessQosPolicyKind kind);

	/**
	 * @brief 根据guid获取对应的存活状态管理集合中writer的Liveliness Qos信息。
	 *
	 * @param [in] guid writer的guid。
	 * @param [out] kind 返回获取的Liveliness Qos信息。
	 *
	 * @return 获取成功返回true，否则返回false。
	 */
	bool GetWriterLiveLinessKind(GUID_t guid, LivelinessQosPolicyKind& kind);

	/**
	 * @brief 检查给定类型的任何writer是否活动的函数。
	 *
	 * @param [in] kind Liveliness Qos信息。
	 *
	 * @return 对应writer存活则返回true，否则返回false。
	 */
    bool HasAliveWriters(LivelinessQosPolicyKind kind);

private:

	/**
	 * @brief 更新对应的存活状态管理集合中writer的存活状态。
	 *
	 * @param [in] writer 要更新状态writer。
	 *
	 * @return 更新成功返回true，否则返回false。
	 */
    void AssertWriterLiveliness(LivelinesStatus& writer);

	/**
	* @brief 查找并更新下一个需要检测存活性的writer。
	*
	* @return 查找成功返回true，否则返回false。
	*/
    bool UpDateCheckedWriter();

	/**
	 * @brief 计时器过期时调用的函数。
	 *
	 * @return 执行成功返回true，否则返回false。
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
