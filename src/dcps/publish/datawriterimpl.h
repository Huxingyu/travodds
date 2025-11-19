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
#ifndef TRAVODDS_DCPS_PUBLISH_DATAWRITERIMPL_H
#define TRAVODDS_DCPS_PUBLISH_DATAWRITERIMPL_H

#include <map>

#include "dcps/publish/datawriter.h"
#include "dcps/domain/domainentityimpl.h"
#include "dcps/topic/typesupport.h"
#include "dcps/sql/ddssqlfilter.h"
#include "rtps/historycache/historycache.h"
#ifdef DDSSECURITY
#include "security/crycommonsource.h"
#endif // DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

class Writer;
class TypeSupport;
class PublisherImpl;
class TopicImpl;
class Timer;
class DataWriterStatusManager;

#ifdef DDSSECURITY
struct SecurityPluginInstanceImpl;

struct WriterInfo
{
	// 安全相关
	/** @brief Specify if the DataWriter is enabled security */
	bool enableSec;
	/** @brief The security attributes for the DataWriter */
	EndpointSecurityAttributes secAttributes;
	/** @brief The payload encode attributes */
	CryptoPayloadEncodeAttributes payloadEncodeAttr;
	/** @brief The submessage encode attributes */
	CryptoWriterSubmessageAttributes subMsgEncodeAttr;
	/** @brief The rtps message encode attributes */
	CryptoRtpsMessageAttributes rtpsMsgEncodeAttr;
	/** @brief The encode message length increment constant comparing the origin message */
	unsigned int encodeMsgLenIncConst;
	/** @brief The encode message length multiple comparing the origin message */
	unsigned int encodeMsgMultiple;
	/** @brief Specify if the rtps message should be encoded */
	bool rtpsMsgProtected;
	/** @brief Specify if the message should be encoded */
	bool encryptoMsg;
};

struct ReaderInfo
{
	ReaderInfo()
		:guid(GUID_UNKNOWN),
		rdData(NULL),
		//epRef(NULL),
		ackNackCount(0),
		fragNackCount(0)
	{
		readyForSecComm = false;
	}
	GUID_t guid;
	const DiscoveredReaderData* rdData;
	//RemoteDREndpoint* epRef;
	SequenceNumber_t ackedSN;
	SequenceNumber_t ackedBatchSN;
	uint32_t ackNackCount;
	uint32_t fragNackCount;
	SequenceNumber_t maxNackSeq;
	//HeartbeatInfo hbInfo;
	/** @brief The remote DataReader crypto handle */
	DatareaderCryptoHandle cryptoHdl;
	/** @brief Whether the reader is ready for security communications */
	bool readyForSecComm;
};

// 快速查找指定Guid的Reader
// 快速获取所有地址的列表
// 快速获取和修改所有Reader的最小SequenceNumber
struct RemoteReaderList
{
	std::map<GUID_t, ReaderInfo*> reliableReaders;
	std::map<GUID_t, ReaderInfo*> besteffortReaders;
	std::map<GUID_t, ReaderInfo> preparingReaders;
	std::map<GUID_t, CryptoTokenSeq> remoteReaderCryptoToken;
};
#endif // DDSSECURITY

/**
 * @brief DataWriterImpl类 包含DataWriter类数据写者功能的实际实现。
 */
class DataWriterImpl : public virtual DataWriter, public DomainEntityImpl {
public:

	/**
	 * @brief 构造函数。
	 *
	 * @param [in] topic       TopicDescription指针。
	 * @param [in] qos	       DataReader Qos。
	 * @param [in] listener	   数据写者监听器。
	 * @param [in] mask		   StatusMask位掩码。
	 * @param [in] pub	       PublisherImpl指针。
	 * @param [in] typeSupport typeSupport指针。
	 */
	DataWriterImpl(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, StatusMask mask, PublisherImpl* pub, TypeSupport* typeSupport);

	/**
	 * @brief 析构函数。
	 */
    ~DataWriterImpl();

	/**
	 * 为此DataWriter设置DataWriterQos。
	 *
	 * @param [in] qos 要设置的DataWriterQos
	 * 
	 * @return RETCODE_IMMUTABLE_POLICY（如果无法更改任何Qos），
	 * RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
	 * RETCODE_OK（如果Qos更改正确）。
	 */
    ReturnCode_t set_qos(const DataWriterQos& qos);

	/**
	 * 使用此DataWriter的Qos值给DataWriterQos变量赋值。
	 *
	 * @param [in] qos 返回qos的DataWriterQos对象。
	 * 
	 * @return RETCODE_OK。
	 */
    ReturnCode_t get_qos(DataWriterQos& qos) const;

	/**
     * @brief 修改DataWriterListener。
     *
     * @param [in] listener DataWriterListener的新值。
     * @param [in] mask 保存监听器响应的状态的StatusMask位掩码。
     * 
     * @return RETCODE_OK
     */
    ReturnCode_t set_listener(DataWriterListener* listener, const StatusMask& mask);

	/**
	 * @brief 检索此DataWriter的监听器。
	 *
	 * @return 指向DataWriterListener的指针。
	 */
    const DataWriterListener* get_listener() const;

	/*
	 * @brief 通知应用程序将修改特定实例。
	 * 它为中间件提供了一个预先配置自己以提高性能的机会。
	 *
	 * @param[in] instance_data 用于获取实例键值的示例。
	 *
	 * @return 包含实例键的句柄。
	 * 该句柄可以用于连续的“写入”或“释放”函数。
	 * 如果出现错误，将返回HANDLE_NIL。
	 */
	InstanceHandle_t register_instance(const void* const instance_data);

	/**
	 * @brief 此函数执行与register_instance相同的函数，并且在应用程序希望指定
	 * 源时间戳source_timestamp的值的情况下，可以使用此函数代替register_instance。
	 * 源时间戳source_ttimestamp可能会影响相对数据读者检测接收的多个数据写者的数据的顺序。
	 * 请参阅QoS策略destination_order 的“DESTINATION_ORDER”选项。
	 *
	 * 在与write函数相同的情况下，此函数可能会阻止并返回RETCODE_TIMEOUT。
	 *
	 * 在write函数所述的相同情况下，此函数可能会返回RETCODE_OUT_OF_RESOURCES。
	 *
	 * @param [in] instance_data    用于获取实例键值的数据。
	 * @param [in] source_timestamp 用于设置源时间戳。
	 * @return 包含实例键的句柄。
	 */
	InstanceHandle_t register_instance_w_timestamp(const void* const instance_data, const Time_t& source_timestamp);

	/**
	 * @brief 此函数为 register_instance 函数的注销函数。
	 * 它应该仅在当前注册的实例上调用。
	 * 通知中间件DataWriter不打算再修改该数据实例。
	 * 还指示中间件可以在本地删除与该实例相关的所有信息。
	 *
	 * @param[in] instance 在`handle`参数的情况下，用于推断实例键的示例是HANDLE_NIL。
	 * @param[in] handle 要注销的实例的键值。
	 * @return 返回函数的执行结果。
	 * 如果函数执行成功完成，则返回RETCODE_OK。
	 */
    ReturnCode_t unregister_instance(const void* const instance_data, const InstanceHandle_t& handle) ;

	/**
	 * @brief 此函数执行与unregister_instance相同的函数，可以代替
	 * unregister_instance，在应用程序希望指定源时间戳source_timestamp。
	 * 源时间戳source_timestamp可能会影响相对数据读者检测接收的多个数据写者的数据的顺序。
	 * 请参阅QoS策略destination_order 的“DESTINATION_ORDER”。
	 *
	 * handle 参数值的约束和相应的错误行为与为unregister_instance函数指定的约束相同。
	 *
	 * 在与write函数在相同的情况下，该函数可能会阻塞并返回RETCODE_TIMEOUT
	 *
	 * @param [in] instance_data    在`handle`参数的情况下，用于推断实例键的示例是handle_NIL。
	 * @param [in] handle           要注销的实例的键值。
	 * @param [in] source_timestamp 用于设置源时间戳。
	 * 
	 * @return 返回函数的执行结果。
	 * 如果函数执行成功完成，则返回RETCODE_OK。
	 */
	ReturnCode_t unregister_instance_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	/**
	 * @brief 此函数可用于检索与instance_handle对应的实例键值。
	 * 该函数将仅填充在key_holder实例内形成键的字段。
	 *
	 * 如果InstanceHandle_t句柄与DataWriter已知的现有数据对象不对应，
	 * 则此函数可能会返回BAD_PARAMETER。
	 * 如果实现无法检查无效句柄，则未指定这种情况下的结果。
	 *
	 * @param[in,out] key_holder
	 * @param[in]     handle
	 *
	 * @return 根据执行情况返回任何标准返回代码。
	 */
	ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

	/**
	 * @brief 将实例作为参数，并返回可在接受实例句柄作为参数的后续函数中使用的句柄。
	 * 实例参数仅用于检查键值的字段。
	 *
	 * @param [in] instance_data 实例数据指向示例的指针。
	 *
	 * @return 实例对应的键值句柄
	 */
	InstanceHandle_t lookup_instance(const void* const instance_data) const;

	/**
	 * @brief 使用句柄发送数据。
	 *
	 * 特殊值HANDLE_NIL可以用于参数句柄。这表示应该依靠键值从instance_data自动推断实例的标识。
	 *
	 * @param [in] instance_data 指向数据的指针
	 * @param [in] handle InstanceHandle_t.
	 * @return 如果引入的句柄与与数据关联的句柄不匹配返回RETCODE_PRECONDITION_NOT_MET，
	 * 如果数据正确发送，则返回RETCODE_OK，否则返回RETCCODE_ERROR。
	 */
	ReturnCode_t write(const void* const instance_data, const InstanceHandle_t& handle);

	/**
	 * @brief 此函数执行与写入相同的功能，只是它还提供了源时间戳source_timestamp的值，
	 * 该值通过SampleInfo内的source_timestapp属性source_timestamp可用于DataReader对象。
	 * handle 参数值的约束和相应的错误行为对于write 函数是相同的。
	 * 在与write 函数相同的情况下，此函数可能会阻止并返回RETCODE_TIMEOUT。
	 * 在与write函数描述的相同情况下，此函数可以返回RETCODE_OUT_OF_RESOURCES、RETCOD_EPRECONDITION_NOT_MET或RETCOD_BAD_PARAMETER。
	 *
	 * @param [in] instance_data    指向数据的指针
	 * @param [in] handle           InstanceHandle_t
	 * @param [in] source_timestamp 源时间戳。
	 *
	 * @return 任何标准返回代码。
	 */
	ReturnCode_t write_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	/**
	 * @brief 该函数请求中间件删除数据（实际删除被推迟，直到该数据在整个系统中不再使用）。
	 * 通常，应用程序通过对已经知道该实例的DataReader对象的函数来了解删除。
	 * 此函数不会修改实例的值。传递实例参数只是为了标识实例。
	 * 使用此函数时，服务将自动提供source_timestamp的值，该值通过SampleInfo中的source_timestamp属性提供给DataReader对象。
	 * 句柄参数值的约束和相应的错误行为与为unregister_instance函数指定的约束相同。
	 *
	 * @param[in] instance_data 在`handle`参数的情况下，用于推断实例键的示例是否为handle_NIL。
	 * @param[in] handle        数据的InstanceHandle
	 *
	 * @return RETCODE_PRECONDITION_NOT_MET，如果引入的句柄与与数据关联的句柄不匹配，
	 * 如果数据正确发送，则返回RETCODE_OK，否则返回RETCCODE_ERROR。
	 */
	ReturnCode_t dispose(const void* const instance_data, const InstanceHandle_t& handle);

	/**
	 * @brief 此函数执行与dispose相同的函数，只是应用程序提供了原时间戳source_timestamp的值，
	 * 该值通过SampleInfo内的source_Timetamp属性可用于DataReader对象。
	 *
	 * handle参数值的约束和相应的错误行为与为@ref释放函数指定的约束相同。
	 *
	 * 在与dispose函数所述的相同情况下，此函数可以返回RETCODE_PRECONDITION_NOT_MET和RETCOD_EBAD_PARAMETER。
	 *
	 * 在与write函数所述的相同情况下，此函数可以返回RETCODE_TIMEOUT和RETCODE.OUT_OF_RESOURCES。
	 *
	 * @param [in] instance_data    在`handle`参数的情况下，用于推断实例键的示例是handle_NIL。
	 * @param [in] handle           要释放的实例的键值。
	 * @param [in] source_timestamp 用于设置源时间戳的Time_t类型值。
	 *
	 * @return 根据执行情况返回任何标准返回代码。
	 */
	ReturnCode_t dispose_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	/**
	 * @brief 等待当前线程，直到所有写入程序都收到其确认。
	 *
	 * @param [in] max_wait 此函数的最大阻塞时间。
	 *
	 * @return 如果DataWriter在时间到期之前收到确认返回RETCODE_OK，否则为RETCODE_ERROR。
	 */
	ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait);

	/**
	 * @brief 返回Liveliness状态。
	 *
	 * @param [in] status 活性丢失状态结构体变量。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t get_liveliness_lost_status(LivelinessLostStatus& status);

	/**
	 * @brief 返回提供的错过截止日期状态。
	 *
	 * @param [out] status 截止日期未命中状态结构体变量。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t get_offered_deadline_missed_status(OfferedDeadlineMissedStatus& status);

	/**
	 * @brief 返回提供的不兼容qos状态。
	 *
	 * @param [out] status 提供的不兼容qos状态结构体变量。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus& status);

	/**
	 * @brief 返回发布匹配状态。
	 *
	 * @param [out] status 发布匹配的状态结构体变量。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t get_publication_matched_status(PublicationMatchedStatus& status);

	/**
	 * @brief 获取此DataWriter的主题。
	 *
	 * @return 指向对应主题的指针。
	 */
	Topic* get_topic() const;

	/**
	 * @brief 获取创建此数据写者的发布者。
	 *
	 * @return 指向对应发布者的指针。
	 */
	const Publisher* get_publisher() const;

	/**
	 * @brief 此函数手动声明DataWriter的活动性。这与LivelinessQosPolicy结合使用，以向服务指示实体保持活动状态。
	 * 仅当LIVELINESS设置为MANUAL_BY_PARTICIPANT或MANUAL_BY_TOPIC时，才需要使用此函数。否则，它没有效果。
	 *
	 * @note 通过DataWriter上的write函数发送数据会声明DataWriter本身及其DomainParticipant上的活跃性。
	 * 因此，只有当应用程序没有定期发送数据时，才需要使用assert_livelity函数。
	 *
	 * @return 如果声明成功返回RETCODE_OK，否则返回RETCOD_ERROR。
	 */
	ReturnCode_t assert_liveliness();

	/**
	 * @brief 在与DataWriter关联的订阅者中检索。
	 *
	 * @param [out] subscription_data 订阅者信息数据结构。
	 * @param [in]  subscription_handle 要获取信息的订阅者的InstanceHandle_t。
	 *
	 * @return RETCODE_OK。
	 *
	 * @warning Not supported yet. Currently returns RETCODE_UNSUPPORTED
	 */
	ReturnCode_t get_matched_subscription_data(SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle);
	
	/**
	 * @brief 获取对应的数据读者的InstanceHandle_t。
	 *
	 * @param [out] subscription_handles 返回InstanceHandle_t的容器。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t get_matched_subscriptions(InstanceHandleSeq& subscription_handles);
#ifdef _XML_INTERFACE_CPP
	/**
	 * @brief 指定QoS仓库的Qos配置，并设置数据写入器的QoS。
	 *
	 * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
	 * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
	 * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
	 *
	 * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t set_qos_with_profile(
		const char* library_name,
		const char* profile_name,
		const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
	/**
	 * @brief 此函数用于启用实体
	 *
	 * @return 启用成功返回RETCODE_OK，
	 *         不满足重新编码前提条件时返回RETCODE_PRECONDITION_NOT_MET，
	 *         启用失败返回RETCODE_ERROR。
	 */
	ReturnCode_t enable();

	/**
	 * @brief 设置相关数据写者。
	 *
	 * @param [in] writer Writer指针。
	 */
	void SetRelatedWriter(std::shared_ptr<Writer> writer);

	/**
	 * @brief 更新活跃度失活状态。
	 * 
	 * @return 更新成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t UpdateLivelinessLostStatus();

	/**
	 * @brief 取消注册或处置
	 * 实现unregister_instance和dispose函数具体功能。
	 * 
	 * @param [in] data		 在`handle`参数的情况下，用于推断实例键的示例是handle_NIL。
	 * @param [in] handle	 要释放的实例的键值。
	 * @param [in] timespace 用于设置源时间戳的Time_t类型值。
	 * @param [in] dispose	 dispose函数调用标志位。
	 * 
	 * @return 更新成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t UnRegisteOrDispose(const void* const data, const InstanceHandle_t& handle, const Time_t& timespace, bool dispose = false);

	//void OnLivelinessLost(DataWriter* the_writer, const LivelinessLostStatus& status);

	//void OnOfferedDeadlineMissed();

	/**
	 * @brief 更新通信请求的Qos不兼容情况。
	 *
	 * @param [in] qosSeq 不兼容的Qos的ID。
	 */
	void OnOfferedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq);

	/**
	 * @brief 更新发布者匹配状态信息。
	 *
	 * @param [in] handle 远端Reader句柄。
	 * @param [in] add    是否为新增的远端Reader。
	 */
	void OnPublicationMatched(InstanceHandle_t& handle, const bool& add = true);

	 /**
	  * @brief 获取内置主题数据参数。
	  *
	  * @return 内置主题数据参数。
	  */
    const PublicationBuiltinTopicData& GetBuiltinTopicData();
#ifdef DDSSECURITY
	const PublicationBuiltinTopicSecureData& GetBuiltinTopicSecureData();
#endif // DDSSECURITY

	/**
	 * @brief 用户数据键值生成函数。
	 *
	 * @param [in]  data     用户报文指针。
	 * @param [out] handle   根据报文生成的带有键值的句柄。
	 * @param [in]  forceMd5 是否强制进行MD5转换标志。
	 *
	 * @return 生成成功返回true，否则返回false。
	 */
	bool MakeKey(const void* const data, InstanceHandle_t& handle, bool forceMd5) const;
	
	/**
	 * @brief Qos策略中deadline Qos相关定时器超时执行函数。
	 */
	void DeadlineTimeout();

	/**
	 * @brief 删除过期样本的方法，在lifespan Qos相关定时器超时执行函数。
	 */
	bool LifespanTimeout();

	/**
	 * @brief 注册实例。
	 *
	 * @param [in]  instance_data    用户报文指针。
	 * @param [in]  source_timestamp 源时间戳。
	 * @param [out] handle			 根据报文生成的带有键值的句柄。
	 *
	 * @return 注册成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t RegistInstance(const void* const instance_data, const Time_t& source_timestamp, InstanceHandle_t& handle);

	/**
	 * @brief 根据数据的长度，创建对应的SerializedBuffer。
	 *
	 * @param [out] bufferHeader 用户报文指针。
	 * @param [in]  dataLen      报文长度。
	 *
	 * @return 创建成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t CreateSerializedBuffer(SerializedBuffer* &bufferHeader, uint32_t dataLen);
	
	/**
	 * @brief 释放对应的空间。
	 *
	 * @param [in]  buffer 用户报文指针。
	 *
	 * @return 释放成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t DeleteSerializedBuffer(SerializedBuffer* buffer);

	/**
	 * @brief 添加Reader过滤器。
	 *
	 * @param [in] readerGuid 读者的GUID。
	 * @param [in] filterInfo 过滤器信息。
	 */
	bool AddReaderFilter(const GUID_t& readerGuid, const ContentFilterProperty_t& filterInfo);

	/**
	 * @brief 移除Reader过滤器。
	 *
	 * @param [in] readerGuid 读者的GUID。
	 * 
	 */
	void RemoveReaderFilter(const GUID_t& readerGuid);

#ifdef DDSSECURITY
	PublisherImpl* GetParent();

	uint32_t SetRemoteReaderCryptoTokens(const GUID_t* readerId, const CryptoTokenSeq* tokens);

	ReturnCode_t GetSecAttributes();

	/**
	 * @brief根据加载的属性填充builtinSecureData_的成员。
	 *
	 */
	ReturnCode_t FillSecureData();
#endif // DDSSECURITY
	
private:
    TopicImpl* topic_ = nullptr;
    DataWriterQos qos_;
    DataWriterListener* listener_ = nullptr;
    PublisherImpl* pub_ = nullptr;
	/* 相关联的RTPS对象 */
	std::shared_ptr<Writer>  writer_;
	
	std::map<GUID_t, std::shared_ptr<DDSSQL::DDSSQLFilter>> readerFilters_;

	/* 相关的数据类型支持的对象  */
	std::unique_ptr<DataWriterStatusManager> statusMgr_;
	std::shared_ptr<Timer> deadlineTimer_;
	InstanceHandle_t lastInstance_;
	mutable std::mutex lastInstanceMtx_;
	/* wangyi lifespan 定时器 */
	std::shared_ptr<Timer> lifespanTimer_;

    PublicationBuiltinTopicData builtinData_;

#ifdef DDSSECURITY
public:
	PublicationBuiltinTopicSecureData builtinSecureData_;
	WriterInfo statistic_;
	/** @brief The crypto handle for the DataWriter */
	DatawriterCryptoHandle cryptoHdl_;
	/** @brief The security plugin instance */
	SecurityPluginInstanceImpl* secPlugin_ = NULL;
	/** @brief The list of DataReader which are enabled security */
	std::vector<DatareaderCryptoHandle> secReaders_;

	SecurityPluginInstanceImpl* dpSecPlugin_;
	//TODO 安全扩展，维护几个远端的guid和信息的map
	RemoteReaderList remoteReaders_;
#endif // DDSSECURITY

	std::shared_ptr<MemoryPool> memoryPool_;
protected:
	TypeSupport* typeSupport_ = nullptr;

	std::shared_ptr<HistoryCache> history_ = nullptr;
};
TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_DCPS_PUBLISH_DATAWRITERIMPL_H
