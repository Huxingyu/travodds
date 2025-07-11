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
#ifndef TRAVODDS_RTPS_ENDPOINT_READERLOCATOR_H
#define TRAVODDS_RTPS_ENDPOINT_READERLOCATOR_H

#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * ReaderLocator类,包含有关远端DataReader的信息，但不保存其状态。
 * （尽力通信模式）
 */
class ReaderLocator {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] locator DataReader地址信息。
	 */
	ReaderLocator(const Locator_t& locator);

	/**
	 * @brief 获取下一个请求报文的序号。
	 *
	 * @return 下一个请求报文的序号。
	 */
	SequenceNumber_t NextRequestedChange();

	/**
	 * @brief 获取下一个未发送报文的序号。
	 *
	 * @return 下一个未发送报文的序号。
	 */
	SequenceNumber_t NextUnsentChange();

	/**
	 * @brief 获取所有请求报文的序号。
	 *
	 * @return 所有请求报文的序号列表。
	 */
	SeqNumberVec RequestedChanges();

	/**
	 * @brief 设置请求报文的序号列表。
	 *
	 * @param [in] reqSeqNumSet 请求报文的序号列表。
	 */
	void RequestedChangesSet(SeqNumberVec reqSeqNumSet);

	/**
	 * @brief 判定是否存在未发送报文请求。
	 *
	 * @return 是则返回true，否则返回false。
	 */
	bool UnsentChanges();

	/**
	 * @brief 设置要发送的报文缓冲区。
	 *
	 * @param [in] writerCache 请求报文缓冲区指针。
	 */
	void SetHistoryCache(std::shared_ptr<HistoryCache> writerCache);

	/**
	 * @brief 重置最高发送请求项。
	 *
	 * @return 重置成功返回true，否则返回false。
	 */
	bool ResetHighestSentChange();

	/**
	 * @brief 获取远端DataReader地址信息。
	 *
	 * @return 远端DataReader地址信息。
	 */
	Locator_t GetLocator();

	//bool sendData();
private:
	SequenceNumber_t highestSentChangeSN_;
	SeqNumberVec requestedChanges_;
	Locator_t locator_;
	bool expectsInlineQos_;

	std::shared_ptr<HistoryCache> writerCache_ = nullptr;
};
TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_ENDPOINT_READERLOCATOR_H
