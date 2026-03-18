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
#ifndef TRAVODDS_DCPS_SUBSCRIBE_DATAREADERIMPL_H
#define TRAVODDS_DCPS_SUBSCRIBE_DATAREADERIMPL_H

#include "dcps/subscribe/datareader.h"
#include "dcps/domain/domainentityimpl.h"
#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"
#include "dcps/sql/ddssqlfilter.h"

#ifdef DDSSECURITY
#include "security/crycommonsource.h"
#endif // DDSSECURITY

TRAVODDS_NAMESPACE_BEGIN
class SerializedBuffer;
class Reader;
class SubscriberImpl;
class TopicDescriptionImpl;
class TypeSupport;
class DataReaderStatusManager;
class Timer;
class ReadConditionImpl;
class QueryConditionImpl;

#ifdef DDSSECURITY
class Spdp;
class SecurityPluginInstanceImpl;

typedef struct WriterSampleList
{
	/** @brief   DataWriter的标识. */
	GUID_t writerId;
	/** @brief DomainParticipant所使用的加密/解密句柄 */
	ParticipantCryptoHandle partCryptoHdl = NULL;
	/** @brief 该DataWriter所使用的加密/解密句柄 */
	DatawriterCryptoHandle cryptoHdl = NULL;
	/** @brief 指明该DataWriter是否已经准备好进行通信的加密/解密，即是否已经完整获取该DataWriter的安全相关属性 */
	bool readyForSecComm = false;
} WriterSampleList;
#endif // DDSSECURITY

/**
 * @brief DataReaderImpl类，DataReader类功能实现类，包含数据读者行为的实际实现。
 */
class DataReaderImpl : public virtual DataReader, public DomainEntityImpl {
public:

	/**
	 * @brief 构造函数。
	 *
	 * @param [in] topic       TopicDescription指针。
	 * @param [in] qos	       DataReader Qos。
	 * @param [in] listener	   数据读者监听器。
	 * @param [in] mask		   状态掩码。
	 * @param [in] sub	       SubscriberImpl指针。
	 * @param [in] typeSupport typeSupport指针。
	 */
	DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport);

	/**
	 * @brief 析构函数。
	 */
	~DataReaderImpl();

	/**
	 * @brief 要设置的DataReaderQos策略。
	 *
	 * @param [in] qos 新的DataReaderQos策略.
	 *
	 * @return RETCODE_IMMUTABLE_POLICY（如果无法更改任何Qos），
	 * RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
	 * RETCODE_OK（如果Qos更改正确）。
	 */
	virtual ReturnCode_t set_qos(const DataReaderQos& qos);

	/**
	 * @brief 获取当前DataReaderQos策略。
	 *
	 * @param [in] qos 返回的当前DataReader Qos策略。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_qos(DataReaderQos& qos) const;

	/**
	 * 设置DataReaderListener。
	 *
	 * @param [in] listener 新的DataReaderListener指针。
	 * @param [in] mask 保存监听器响应的状态StatusMask对象.
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t set_listener(DataReaderListener* listener, const StatusMask& mask);

	/**
	 * @brief 获取DataReaderListener。
	 *
	 * @return 指向DataReaderListener的指针。
	 */
	virtual const DataReaderListener* get_listener() const;

	/**
	 * 从DataReader访问数据样本的集合。
	 *
	 * 此操作从DataReader访问数据值的集合。调用者可以通过max_samples参数限制返回的集合的使用大小。
	 *
	 * data_values集合的属性和PresentationQosPolicy的设置可能会对返回的“列表”的大小施加进一步的限制。
	 *
	 * 1.如果PresentationQosPolicy::access_scope是INSTANCE_PRESENTATION_QOS，
	 *   则返回的集合是一个“列表”，其中属于同一数据实例的样本是连续的。
	 *
	 * 2.如果PresentationQosPolicy::access_scope是TOPIC_PRESENTATION_QOS，
	 *   而PropentationQOSPolicy::ordered_access设置为false，
	 *   则返回的集合是一个“列表”，其中属于同一数据实例的样本是连续的。
	 *
	 * 3.如果PresentationQosPolicy::access_scope是TOPIC_PRESENTATION_QOS，
	 *   而PropentationQOSPolicy::ordered_access设置为true，则返回的集合是一个“列表”，
	 *   其中属于同一实例的样本可能是连续的，也可能不是连续的。这是因为为了保持秩序，可能需要混合来自不同实例的样本。
	 *
	 * 4.如果PresentationQosPolicy::access_scope是GROUP_PRESENTATION_QOS，
	 *   并且PropentationQOSPolicy::ordered_access设置为false，
	 *   则返回的集合是一个“列表”，其中属于同一数据实例的样本是连续的。
	 *
	 * 5.如果PresentationQosPolicy::access_scope为GROUP_PRESENTATION_QOS，
	 *   并且PropentationQOSPolicy::ordered_access设置为true，则返回的集合最多包含一个样本。
	 *   这种情况下的差异是由于要求应用程序能够以特定顺序读取属于不同DataReader对象的样本。
	 *
	 * 在任何情况下，一个实例的样本之间的相对顺序都与DestinationOrderQosPolicy一致：
	 *
	 * -如果DestinationOrderQosPolicy::kind为BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS，
	 *  则属于相同实例的样本将以接收的相对顺序出现（FIFO，较早的样本先于较晚的样本）。
	 *
	 * -如果DestinationOrderQosPolicy::kind为BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS，
	 *  则属于相同实例的样本将以SOURCE_TIMESTAMP所标识的相对顺序出现（FIFO，SOURCE_TIMESTAMP的较小值先于较大值）。
	 *
	 * 返回的实际样本数取决于中间件以及HistoryQosPolicy、ref ResourceLimitsQosPolic和ReaderResourceLimitesQos接收到的信息：
	 *
	 * -HistoryQosPolicy::kind为KEEP_LAST_HISTORY_QOS的情况下，调用函数后将根据HistoryQosPolicy::depth值返回每个实例的样本个数。
	 *
	 * -返回的最大样本数受ResourceLimitsQosPolicy::max_samples和ReaderResourceLimitesQos::max_samples_per_read的限制。
	 *
	 * -对于多个实例，返回的样本数还受结果
	 * （ResourceLimitsQosPolicy::max_samples_per_instance*ResourceLimitsQosPolicy::max_instances）值的限制。
	 *
	 * -如果ReaderResourceLimitsQos::sample_infos_allocation具有最大限制，
	 *  则如果SampleInfo资源不足，则返回的样本数也可能有限。
	 *
	 * 如果操作成功，并且返回的样本数受到限制（通过上面列出的最大限制或SampleInfo资源不足），
	 * 则调用将成功完成，并提供读取器能够返回的样本。用户可能需要进行额外的调用，
	 * 或者在资源不足的情况下返回未完成的借用缓冲区，以便访问剩余的样本。
	 *
	 * 除了样本集合之外，读取操作还使用SampleInfo结构的集合（sample_infos）。
	 *
	 * data_values和sample_infos集合的初始（输入）属性将确定此操作的精确行为。
	 * 在本说明中，创建的集合具有以下三个属性：
	 *
	 * -当前长度（len，请参见LoanableCollection::length()）
	 *
	 * -最大长度（max_len，请参见LoanableCollection::maximum()）
	 *
	 * -集合容器是否拥有中元素的内存（owns，请参见LoanableCollection::has_ownership()）
	 *
	 * len、max_len和owns参数的初始（输入）值对data_values和sample_infos集合的属性的控制及read函数的行为，如以下规则所指定：
	 *
	 * 1.两个集合的len、max_len和owns的值必须相同。否则，读取将失败，并返回RETCODE_PRECONDITION_NOT_MET。
	 *
	 * 2.成功输出后，两个集合的len、max_len和owns的值将相同。
	 *
	 * 3.如果输入<tt>max_len == 0 </tt>，则data_values和sample_infos集合将由DataReader“借用”的元素填充。
	 *   在输出时，owns将为false，len将设置为返回的值的数量，max_len将被设置为验证<tt>max_len>=len</tt>的值。
	 *   使用此变体允许对数据进行零拷贝访问，并且应用程序需要使用return_loan操作将“借用”的元素返回给DataReader。
	 *
	 * 4.如果输入<tt>max_len > 0 </tt>且输入 <tt> owns == false </tt>，则read操作将失败，
	 *   并返回RETCODE_PRECONDITION_NOT_MET。这避免了由于应用程序忘记归还“借用”的元素而导致的潜在的难以检测的内存泄漏。
	 *
	 * 5. 如果输入<tt>max_len > 0</tt>并且输入<tt> owns == true </tt>.则read操作将把Data值和SampleInfo值复制到集合中已经存在的元素中。
	 *    在输出时，owns将为true，len将设置为复制的值的数量，max_len将保持不变。
	 *    使用此变体会强制执行副本，但应用程序可以控制副本的放置位置，并且应用程序不需要归还“借用”的元素。
	 *    复制的采样数取决于max_len和max_samples的值：
	 *
	 *      -如果<tt> max_samples == LENGTH_UNLIMITED</tt>，则最多复制max_len值。
	 *       该变体的使用允许应用程序将返回的样本数量限制为序列可以容纳的数量。
	 *
	 *      -如果<tt> max_samples <= max_len </tt>，则最多复制max_samples值。
	 *       使用此变体可以让应用程序将返回的样本数限制为序列可以容纳的更少。
	 *
	 *      -如果<tt> max_samples > max_len </tt>，则读取操作将失败，并返回RETCODE_PRECONDITION_NOT_MET。
	 *       这避免了应用程序期望能够访问多达max_samples的潜在混乱问题，但该数字永远不能返回，
	 *       即使它们在DataReader中可用，因为输出序列无法容纳它们。
	 *
	 * 如上所述，在返回时，data_values和sample_infos集合可以包含从DataReader“借用”的元素。
	 * 如果是这种情况，一旦应用程序不再在集合中使用Data，则需要使用return_loan操作来返回这些元素。
	 * 从return_loan返回时，集合将具有<tt> max_len == 0 </tt>和<tt> owns == false </tt>特性。
	 *
	 * 应用程序可以根据调用读取操作时集合的状态，或通过访问owns属性来确定是否需要返回贷款。
	 * 然而，在许多情况下，始终调用return_loan可能更简单，因为如果集合没有“借用”，则此操作是无其他影响的（即，保持所有元素不变）。
	 *
	 * 在输出时，数据值的集合和SampleInfo结构的集合具有相同的长度，并且是一对一的对应关系。
	 * 每个SampleInfo提供有关相应样本的信息，例如source_timestamp、sample_state、view_state和instance_state等。
	 *
	 * 返回集合中的某些元素可能没有有效的数据。如果SampleInfo中的instance_state是NOT_ALIVE_DISPOSED_INSTANCE_STATE或
	 * NOT_ALIVE_NO_WRITERS_INSTANCE_STATE。则集合中该实例的最后一个样本，即SampleInfo==0的样本不包含有效数据。
	 * 不包含数据的样本不会计入在ResourceLimitsQosPolicy中施加的限制。
	 *
	 * 读取样本的操作将其sample_state更改为READ_SAMPLE_STATE。
	 * 如果样本属于实例的最新生成，它还将实例的view_state设置为NOT_NEW_VIEW_STATE。它不会影响实例的instance_state。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @em 注意：如果此方法“返回”的样本是从中间件借用的（有关内存借用的更多信息，请参阅take），重要的是不要更改其内容。
	 * 由于存储数据的存储器属于中间件，因此下次读取或采集相同的样本时，将看到对数据所做的任何修改；样本将不再反映从网络接收的状态。
	 *
	 * @param [in,out] data_values     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sample_infos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     max_samples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将返回尽可能多的样本，直到达到上述限制。
	 * @param [in]     sample_states   只有sample_state与其中一个匹配的数据样本才会返回。
	 * @param [in]     view_states     只有view_state与其中一个匹配的数据样本才会返回。
	 * @param [in]     instance_states 仅返回instance_state与其中一个匹配的数据样本。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 从DataReader访问数据样本的集合。
	 *
	 * 此操作访问DataReader中的数据样本集合和相应的SampleInfo结构集合，并从DataReader中“删除”它们。
	 * 该操作将返回样本的“列表”，否则返回单个样本。这由PresentationQosPolicy使用与read操作相同的逻辑来控制。
	 *
	 * 获取样本的操作会将其从DataReader中删除，因此它不能再次“读取”或“获取”。
	 * 如果样本属于实例的最新生成，则它还将实例的view_state设置为NOT_NEW。它不会影响实例的instance_state。
	 *
	 * take操作的行为遵循与read操作相同的规则，这些规则涉及data_values和sample_infos集合的前置条件和后置条件。\
	 * 与read类似，take操作可以将元素“借用”到输出集合，然后必须通过return_loan返回这些输出集合。
	 * read的唯一区别是，如上所述，take返回的样本将不再可供后续的读取或take调用访问。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sample_infos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     max_samples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将返回尽可能多的可用样本，最高可达read()在文档中描述的限制。
	 * @param [in]     sample_states   只有sample_state与其中一个匹配的数据样本才会返回。
	 * @param [in]     view_states     只有view_state与其中一个匹配的数据样本才会返回。
	 * @param [in]     instance_states 仅返回instance_state与其中一个匹配的数据样本。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 此操作通过“读取”匹配ReadCondition中指定的条件的样本来访问。
	 * 此操作与QueryCondition结合使用时特别有用，可以根据内容过滤数据样本。
	 *
	 * 指定的ReadCondition必须附加到DataReader；否则操作将失败，并返回RETCODE_PRECONDITION_NOT_MET。
	 *
	 * 如果ReadCondition仅为ReadCondition类对象.而不是专用的QueryCondition，
	 * 则该操作相当于调用read并以sample_states、view_states和instance_states的形式
	 * 传递a_condition中相应属性的值。使用此操作，应用程序可以避免重复创建ReadCondition时指定的相同参数。
	 *
	 * 使用与读取操作相同的语义访问样本。如果DataReader没有满足约束的样本，则返回值将为RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sample_infos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     max_samples     要返回的最大样本数。
	 * @param [in]     readcondition   返回data_values的ReadCondition必须通过验证
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition);

	/**
	 * 该操作类似于read_w_condition，只是它通过“take”操作访问样本。
	 *
	 * 指定的ReadCondition必须附加到DataReader；否则操作将失败，并返回RETCODE_PRECONDITION_NOT_MET。
	 *
	 * 使用与take操作相同的语义访问样本。
	 *
	 * 此操作与QueryCondition结合使用时特别有用，可以根据内容过滤数据样本。
	 *
	 * If the DataReader has no samples that meet the constraints, the return value will be RETCODE_NO_DATA.
	 *
	 * @param [in,out] data_values     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sample_infos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     max_samples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将按原样返回任意数量的样本。
	 * @param [in]     readcondition   返回data_values的ReadCondition必须通过验证
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition);

	/**
	 * @brief 此操作从DataReader获取下一个、以前未访问的数据值；该操作还获取相应的SampleInfo。
	 * 存储在DataReader中的样本之间的隐含顺序与读取操作的隐含顺序相同。
	 *
	 * read_next_sample操作在语义上等同于读取操作，其中输入数据序列默认有<tt>max_length = 1</tt>、
	 * <tt>sample_states = NOT_READ_SAMPLE_STATE</tt>、<tt>view_states = ANY_VIEW_STATE</tt>
	 * 和<tt>instance_states =ANY_INSTANCE_STATE</tt>设置。
	 *
	 * read_next_sample操作提供了一个简化的API来“读取”样本，从而避免了应用程序管理序列和指定状态的需要。
	 *
	 * 如果DataReader中没有未读数据，则操作将返回RETCODE_NO_DATA，并且不会复制任何内容。
	 *
	 * @param [out] data_value  存储样本的数据指针。
	 * @param [out] sample_info 用于存储示例信息的SampleInfo指针。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read_next_sample(void* data_value, SampleInfo& sample_info);

	/**
	 * @brief 此操作从DataReader获取下一个以前未访问的数据值，并将其从DataReader'删除'，
	 * 因此它不再可访问。该操作还复制相应的SampleInfo。
	 *
	 * 此操作类似于read_next_sample，只是示例已从DataReader中“删除”。
	 *
	 * 该操作在语义上等价于take操作，其中输入数据序列默认有<tt>max_length = 1</tt>、
	 * <tt>sample_states = NOT_READ_SAMPLE_STATE</tt>、<tt>view_states = ANY_VIEW_STATE</tt>
	 * 和<tt>instance_states =ANY_INSTANCE_STATE</tt>设置。
	 *
	 * 该操作提供了一个简化的API来“获取”样本，从而避免了应用程序管理序列和指定状态的需要。
	 *
	 * 如果DataReader中没有未读数据，则操作将返回RETCODE_NO_DATA，并且不会复制任何内容。
	 *
	 * @param [out] data_value  存储样本的数据指针。
	 * @param [out] sample_info 用于存储示例信息的SampleInfo指针。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take_next_sample(void* data_value, SampleInfo& sample_info);

	/**
	 * 从DataReader访问数据样本的集合。
	 *
	 * 此操作从DataReader访问数据值的集合。该行为与read相同，只是返回的所有样本都属于句柄为a_handle的单个指定实例。
	 *
	 * 成功完成后，数据收集将包含所有属于同一实例的样本。
	 * 相应的SampleInfo验证SimpleInfo::instance_handle == a_handle。
	 *
	 * 此操作在语义上等同于read操作，但在构建集合时除外。
	 * DataReader将检查样本是否属于指定的实例，否则它不会将样本放在返回的集合中。
	 *
	 * 此操作的行为遵循与read操作相同的规则，这些规则涉及data_values和sample_infos的前置条件和后置条件。
	 * 与read类似，此操作可以将元素“借用”到输出集合，然后必须通过return_loan返回输出集合。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sample_infos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     max_samples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
									   则将返回尽可能多的可用样本，最高可达read()在文档中描述的限制。
	 * @param [in]     handle          要返回样本的指定实例。该方法将失败如果句柄与DataReader已知
	 *                                 的现有数据对象不对应，则返回RETCODE_BAD_DATAMETER。
	 * @param [in]     sample_states   只有sample_state与其中之一匹配的数据样本才会被返回。
	 * @param [in]     samole_states   只有view_state与其中之一匹配的数据样本才会被返回。
	 * @param [in]     instance_states 只有instance_state与其中之一匹配的数据样本才会被返回。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 从DataReader访问数据样本集合。
	 *
	 * 此操作从DataReader访问数据值集合，并将其从DataReader中“删除”。
	 *
	 * 此操作与read_instance具有相同的行为，除了样本是从DataReader中“获取”的，
	 * 因此它们不再可以通过后续的“读取”或“获取”操作访问。
	 *
	 * 关于data_values和sample_infos的前置条件和后置条件，此操作的行为遵循与read操作相同的规则。
	 * 与read类似，此操作可能会将元素“借用”到输出集合中，然后必须通过return_loan返还。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     一个LoanableCollection对象，将返回接收到的数据样本。
	 * @param [in,out] sample_infos    一个SampleInfoSeq对象，将返回接收到的样本信息。
	 * @param [in]     max_samples     要返回的最大样本数量。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 将返回尽可能多的可用样本，但不超过read()文档中所述的限制。
	 * @param [in]     handle        指定用于返回样本的实例。若该句柄与DataReader所识别的现有数据对象不匹配，
	 *                                 则此方法将无法执行，返回RETCODE_BAD_PARAMETER。
	 * @param [in]     sample_states   仅匹配这些选项之一的sample_state的数据样本将被返回。
	 * @param [in]     view_states     仅会返回与view_state的值之一相匹配的数据样本。
	 * @param [in]     instance_states 仅会返回与instance_state匹配其中任一值的数据样本。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 访问来自DataReader的数据样本集合。
	 *
	 * 此操作通过数据读取器访问一组数据值，其中所有样本均归属于单个实例。
	 * 其功能与read_instance相似，但并未直接指定实际实例。相反，样本将全部归属于“下一个”实例，
	 * 该实例的instance_handle需大于指定“前一个实例”的handle，并且该实例必须拥有可用样本。
	 *
	 * 此操作意味着实例句柄之间存在一个“大于”关系的全序。此关系的具体细节并不重要，而且取决于实现。
	 * 重要的是，根据中间件，所有实例都是相对于彼此排序的。这种排序是在实例句柄之间进行的，
	 * 不应依赖于实例的状态(e.g.whether是否有数据），并且即使对于不对应于当前由DataReader管理的实例的句柄，
	 * 也必须定义。为了排序的目的，应“仿佛”每个实例句柄都被表示为一个整数。
	 *
	 * 此操作的行为类似于DataReader调用read_instance，并传递最小的instance_handle，
	 * 其中：(a)该instance_handle大于previous_handle，(b)拥有可用的样本（即符合指定状态所施加约束的样本）。
	 *
	 * 特殊值HANDLE_NIL确保其小于任何有效的instance_handle。因此，当使用参数值
	 * previous_handle == HANDLE_NIL时，将返回在所有包含可用样本的实例中，拥有最小instance_handle的实例的样本。
	 *
	 * 此操作用于应用程序驱动的迭代过程中。应用程序首先通过传递previous_handle == HANDLE_NIL启动，
	 * 检查返回的样本。随后，使用SampleInfo中返回的instance_handle作为下一次调用
	 * read_next_instance时的previous_handle参数值。迭代将持续进行，直至read_next_instance返回RETCODE_NO_DATA。
	 *
	 * 请注意，即使使用不对应于当前由数据读取器管理的实例的previous_handle调用read_next_instance操作也是可行的。
	 * 这是因为，如前所述，即便对于不受数据读取器管理的句柄，也明确界定了“大于”关系。一个典型的应用场景是，
	 * 当应用程序遍历所有实例时，会获取NOT_ALIVE_NO_WRITERS_INSTANCE_STATE实例的所有样本，
	 * 随后释放句柄（此时实例信息可能已被移除，导致句柄失效），接着尝试读取下一个实例。
	 *
	 * 此操作的行为遵循与read操作相同的规则，涉及data_values和sample_infos的前置条件和后置条件。
	 * 与read类似，此操作能够“借用”元素至输出集合中，而这些元素随后必须通过return_loan进行归还。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     一个LoanableCollection对象，将返回接收到的数据样本。
	 * @param [in,out] sample_infos    一个SampleInfoSeq对象，将返回接收到的样本信息。
	 * @param [in]     max_samples     要返回的最大样本数量。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 将返回尽可能多的可用样本，但不超过read()文档中所述的限制。
	 * @param [in]     previous_handle 将返回值大于此值且拥有可用样本的“下一个最小”实例。
	 * @param [in]     sample_states   仅匹配这些选项之一的sample_state的数据样本将被返回。
	 * @param [in]     view_states     仅会返回与view_state的值之一相匹配的数据样本。
	 * @param [in]     instance_states 仅会返回与instance_state匹配其中任一值的数据样本。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 访问来自DataReader的数据样本集合。
	 *
	 * 此操作从DataReader中获取数据值集合，并从DataReader中“删除”这些数据值。
	 *
	 * 该操作与read_next_instance的行为一致，区别在于从DataReader中“取出”样本，
	 * 因此这些样本无法通过后续的“读取”或“取出”操作再次访问。
	 *
	 * 与read_next_instance操作类似，可以使用不对应于当前由DataReader管理的实例的previous_handle调用此操作。
	 *
	 * 此操作的行为遵循与read操作相同的规则，涉及data_values和sample_infos的前置条件和后置条件。
	 * 类似于read，此操作可以将元素“借用”到输出集合中，但这些元素随后必须通过return_loan返回。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     一个LoanableCollection对象，将返回接收到的数据样本。
	 * @param [in,out] sample_infos    一个SampleInfoSeq对象，将返回接收到的样本信息。
	 * @param [in]     max_samples     要返回的最大样本数量。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 将返回尽可能多的可用样本，但不超过read()文档中所述的限制。
	 * @param [in]     previous_handle 将返回值大于此值且拥有可用样本的“下一个最小”实例。
	 * @param [in]     sample_states   仅匹配这些选项之一的sample_state的数据样本将被返回。
	 * @param [in]     view_states     仅会返回与view_state的值之一相匹配的数据样本。
	 * @param [in]     instance_states 仅会返回与instance_state匹配其中任一值的数据样本。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * 此操作通过DataReader访问数据值集合。其行为与@ref read_next_instance类似，但返回的所有样本均符合特定条件。
	 * 具体而言，成功时返回的所有样本均属于同一实例，且该实例满足以下两个条件：
	 * (a) instance_handle大于或等于previous_handle；
	 * (b) 在所有具有指定ReadCondition评估为真的样本的实例中，该实例是“最小”的一个。
	 *
	 * 与操作read_next_instance类似，可以使用不对应于当前由DataReader管理的实例的
	 * previous_handle参数来调用read_next_instance_w_condition。
	 *
	 * read_next_instance_w_condition操作的行为遵循与data_values和sample_infos集合读取操作相同的规则，
	 * 包括前置条件和后置条件。与读取操作类似，read_next_instance_w_condition操作可能会“借用”元素至输出集合，
	 * 这些元素随后必须通过return_loan进行归还。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 *
	 * @param [in,out] data_values     一个LoanableCollection对象，将返回接收到的数据样本。
	 * @param [in,out] sample_infos    一个SampleInfoSeq对象，将返回接收到的样本信息。
	 * @param [in]     max_samples     要返回的最大样本数量。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 将返回尽可能多的可用样本，但不超过read()文档中所述的限制。
	 * @param [in]     previous_handle 将返回值大于此值且拥有可用样本的“下一个最小”实例。
	 * @param [in]     readCondition     返回data_values的ReadCondition必须通过验证。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition);

	/**
	 * 此操作通过DataReader访问数据值集合。其行为与read_next_instance类似，区别在于返回的所有样本均符合特定条件。
	 * 具体而言，成功时所有返回的样本均属于同一实例，且该实例是满足以下条件的“最小”实例：
	 * (a) instance_handle的值大于或等于previous_handle，
	 * (b) 具备指定ReadCondition为真结果的样本。
	 *
	 * 与操作read_next_instance类似，可以使用与DataReader当前管理的实例不对应的
	 * previous_handle调用read_next_instance_w_condition。
	 *
	 * read_next_instance_w_condition操作的行为遵循与data_values和sample_infos集合的读取操作相同的规则，
	 * 包括前置条件和后置条件。类似于读取操作，read_next_instance_w_condition操作可能会“借用”元素到输出集合中，
	 * 而这些元素随后必须通过return_loan进行归还。
	 *
	 * 如果DataReader没有满足约束的样本，则操作将失败，并返回RETCODE_NO_DATA。
	 *
	 * @param [in,out] data_values     一个LoanableCollection对象，将返回接收到的数据样本。
	 * @param [in,out] sample_infos    一个SampleInfoSeq对象，将返回接收到的样本信息。
	 * @param [in]     max_samples     要返回的最大样本数量。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 将返回尽可能多的可用样本，但不超过read()文档中所述的限制。
	 * @param [in]     previous_handle 将返回值大于此值且拥有可用样本的“下一个最小”实例。
	 * @param [in]     condition       返回data_values的ReadCondition必须通过验证。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition);

	/**
	 * 此操作告知DataReader，应用程序已结束对通过在DataReader上执行read或take操作前所获取的
	 * data_values和sample_infos集合的访问。
	 *
	 * data_values和sample_infos必须属于同一相关的“对”，即它们应对应于单次读取或采集调用所返回的一对数据。
	 * 此外，data_values和sample_infos还必须从同一个DataReader获取。若上述任一条件未满足，操作将失败，
	 * 并返回RETCODE_PRECONDITION_NOT_MET。
	 *
	 * 此操作支持实现read和@take功能，使得缓冲区可以从DataReader“借出”至应用程序，
	 * 进而提供数据的“零拷贝”访问方式。在“借出”阶段，DataReader将确保数据和采样信息保持不变。
	 *
	 * 应用程序不必在读取或接收呼叫后立即返回“借出”的缓冲区。
	 * 但是，由于这些缓冲区对应于DataReader内部资源，因此应用程序不应无限期保留它们。
	 *
	 * 使用return_loan操作仅在读取或取用“借出”缓冲区提供给应用程序时才需执行。这种情况仅出现在data_values
	 * 和sample_infos集合在调用读取或取用时<tt> max_len == 0 </tt>的情形下。应用程序还可以通过检查集合的
	 * has_ownership属性，来判断是否存在未完成的借出操作。值得注意的是，即使在没有借出的集合上调用return_loan，
	 * 也是安全的，不会引发任何副作用，并且会返回RETCODE_OK。
	 *
	 * 如果集合中有借出记录，则在调用`return_loan`函数返回后，该集合将满足`max_len == 0`的条件。
	 *
	 * @param [in,out] data_values   一个可供借阅的集合对象，其中包含了从该DataReader早期调用read或take方法时所接收的数据样本。
	 * @param [in,out] sample_infos  SampleInfoSeq对象，该对象从DataReader的早期read或take调用中获取了所接收的样本信息。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos);

	/**
	 * 此操作可用于获取与instance_handle对应的实例键值。该操作仅会填充键值持有者实例中的键值字段。
	 *
	 * 如果InstanceHandle_t类型的a_handle不对应于DataReader所知的现有数据对象，则此操作可能会返回BAD_PARAMETER。
	 * 若实现无法验证无效的句柄，则此情况下的结果将是不确定的。
	 *
	 * @param [in,out] key_holder
	 * @param [in] handle
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

	/**
	 * 将实例作为参数传入，并返回一个句柄，该句柄可在后续操作中接受实例句柄作为参数。
	 * 实例参数仅用于检查定义键的字段。
	 *
	 * @param [in] instance 数据指针指向样本。
	 *
	 * @return instance对应的返回句柄。
	 *         如果instance为nullptr，则返回HANDLER_NIL。
	 *         如果没有与instance具有相同键的实例，则返回HANDLE_NIL。
	 */
	virtual InstanceHandle_t lookup_instance(const void* instance_data) const;

	/**
	 * @brief 此操作将生成一个ReadCondition。生成的ReadCondition将自动附加至并隶属于DataReader。
	 *
	 * @param [in] sample_states   仅当sample_state与任一条件相匹配时，数据样本才会触发创建条件。
	 * @param [in] view_states     只有当view_state与这些值之一相匹配时，数据样本才会满足触发创建的条件。
	 * @param [in] instance_states 只有当instance_state符合特定条件，且与预设状态之一相匹配的数据样本，才会触发创建的条件。
	 *
	 * @return 指向创建的ReadCondition，如果出错则为nullptr。
	 */
	virtual ReadCondition* create_readcondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states);

	/**
	 * @brief 此操作将生成一个QueryCondition。生成的QueryCondition将附加至并隶属于DataReader。
	 *
	 * @param [in] sample_states    仅当sample_state与这些条件中的任意一个相匹配时，数据样本才会触发创建的条件。
	 * @param [in] view_states      仅当view_state与这些值中的任一值匹配时，数据样本才会满足触发创建的条件。
	 * @param [in] instance_states  只有当instance_state符合特定条件，且与预设状态之一相匹配的数据样本，才会触发创建的条件。
	 * @param [in] query_expression 只有与该查询匹配的数据样本才会触发创建的条件。
	 * @param [in] query_parameters 查询表达式中参数的值。
	 *
	 * @return 指向创建的QueryCondition，如果出错则为nullptr。
	 */
	virtual QueryCondition* create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const std::string& query_expression, const StringSeq& query_parameters);

	/**
	 * @brief 此操作删除附加到DataReader的ReadCondition类信息。
	 *
	 * @param condition 指向属于DataReader的ReadCondition的指针。
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t delete_readcondition(ReadCondition* condition);

	/**
	 * @brief 获取活跃度变更状态。
	 *
	 * @param [out] status 返回LivelinessChangedStatus状态对象。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_liveliness_changed_status(LivelinessChangedStatus& status) const;

	/**
	 * @brief 获取请求的截止日期已错过状态。
	 *
	 * @param [out] status 返回RequestedDeadlineMissedStatus状态对象。
	 *
	 * @return RETCODE_OK。
	 */
	virtual ReturnCode_t get_requested_deadline_missed_status(RequestedDeadlineMissedStatus& status);

	/**
	 * @brief 获取请求的不兼容的qos状态。
	 *
	 * @param [out] status 要求的qos状态不兼容信息。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus& status);

	/**
	 * @brief 获取SAMPLE_LOST状态。
	 *
	 * @param [out] status 返回包含通信状态的SampleLostStatus对象。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const;

	/**
	 * @brief 获取SAMPLE_REJECTED通信状态
	 *
	 * @param [out] status 返回包含通信状态的SampleRejectedStatus对象。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_sample_rejected_status(SampleRejectedStatus& status) const;

	/**
	 * @brief 返回订阅匹配状态。
	 *
	 * @param [out] status 订阅匹配状态结构。
	 * 
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_subscription_matched_status(SubscriptionMatchedStatus& status) const;

	/**
	 * 获取TopicDescription信息。
	 *
	 * @return TopicDescription指针。
	 */
	virtual const TopicDescription* get_topicdescription() const;

	/**
	 * @brief 获取Subscriber。
	 *
	 * @return Subscriber指针。
	 */
	virtual const Subscriber* get_subscriber() const;

	/**
	 * 此操作将清除通过DataReader的“创建”功能所生成的全部实体。具体而言，
	 * 它将移除所有涵盖的ReadCondition和QueryCondition对象。
	 *
	 * 若所包含的实体处于无法删除的状态，该操作将返回“PRECONDITION_NOT_MET”错误码。
	 *
	 * @return 任何标准返回代码。
	 */
	virtual ReturnCode_t delete_contained_entities();

	/**
	 * @brief 用于暂停当前线程的方法，直至有未读消息可供使用。
	 *
	 * @param [in] max_wait 此操作的最大阻塞时间。
	 * @return 如果有新的未读消息，则返回RETCODE_OK，如果超时，则返回RETCODE_TIMEOUT。
	 *
	 * @warning 尚未支持。当前返回RETCODE_UNSUPPORTED。
	 */
	virtual ReturnCode_t wait_for_historical_data(const Duration_t& max_wait) const;

	/**
	 * @brief 在与DataWriter匹配关联的发布者中检索。
	 *
	 * @param [out] publication_data 发布信息数据结构。
	 * @param publication_handle 发布的InstanceHandle_t。
	 * 
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_matched_publication_data(PublicationBuiltinTopicData& publication_data, const InstanceHandle_t& publication_handle) const;

	/**
	 * @brief 使用匹配的数据读者的InstanceHandle_t填充指定的容器。
	 *
	 * @param [out] publication_handles 返回InstanceHandle_t的容器
	 * @return RETCODE_OK
	 *
	 * @warning 尚未支持。当前返回RETCODE_UNSUPPORTED
	 */
	virtual ReturnCode_t get_matched_publications(InstanceHandleSeq& publication_handles) const;

	/**
	 * @brief 此函数用于启用实体
	 *
	 * @return 启用成功返回RETCODE_OK，
	 *         不满足重新编码前提条件时返回RETCODE_PRECONDITION_NOT_MET，
	 *         启用失败返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t enable();

	virtual uint64_t get_unread_count();

#ifdef _XML_INTERFACE_CPP
	/**
	 * @brief 指定QoS仓库的Qos配置，并设置数据读取器的QoS。
	 *
	 * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
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
	 * @brief 设置相关数据读者。
	 *
	 * @param [in] reader Reader指针。
	 */
	void SetRelatedReader(std::shared_ptr<Reader> reader);

	/**
	 * @brief 判断ReadConditionImpl集合readcdts_参数是否为空。
	 *
	 * @return 为空则返回true，否则返回false。
	 */
	bool Empty() const;

	/**
	 * @brief 解析完成后的用户数据提交。
	 *
	 * @return 为空则返回true，否则返回false。
	 */
	ReturnCode_t OnDataAvaible();

	/**
	 * @brief 更新样本丢失状态。
	 * 
	 * @param [in] handle 样本丢失计数。
	 *
	 * @return 为空则返回true，否则返回false。
	 */
	ReturnCode_t OnSampleLost(int32_t lostCount);

	/**
	 * @brief 更新样本为落选状态并同步状态到用户。
	 *
	 * @param [in] handle 样本句柄。
	 * @param [in] reason 样本落选原因。
	 *
	 * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t OnSampleRejected(const InstanceHandle_t& handle, const SampleRejectedStatusKind& reason);

	//void OnLivelinessChanged(DataReader* the_reader, const LivelinessChangedStatus& status);

	//void OnRequestedDeadlineMissed(DataReader* the_reader, const RequestedDeadlineMissedStatus& status);

	//void OnRequestedIncompatibleQos(DataReader* the_reader, const RequestedIncompatibleQosStatus& status);
	
	/**
	 * @brief 更新通信请求的Qos不兼容情况。
	 *
	 * @param [in] qosSeq 不兼容的Qos的ID。
	 */
	void OnRequestedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq);

	/**
	 * @brief 更新订阅者匹配状态信息。
	 *
	 * @param [in] handle 远端Writer句柄。
	 * @param [in] add    是否为新增的远端Writer。
	 */
	void OnSubscriptionMatched(InstanceHandle_t& handle, const bool& add = true);

	//void OnSampleLost(DataReader* the_reader, const SampleLostStatus& status);

	/**
	 * @brief 更新样本丢失状态。
	 *
	 * @param [in] handle 样本丢失计数。
	 *
	 * @return 为空则返回true，否则返回false。
	 */
	const SubscriptionBuiltinTopicData& GetBuiltinTopicData();
#ifdef DDSSECURITY
	const SubscriptionBuiltinTopicSecureData& GetBuiltinTopicSecureData();

	/**
	 * @brief 获取安全属性。
	 *
	 * @return 成功获取或者所属域参与者未启用安全返回true，否则返回false。
	 */
	bool GetSecAttributes();

	/**
	 * @brief 根据writerId找到匹配的远端写者代理，设置加解密句柄。
	 * 
	 * @param [in] writerId 远端写者Guid。
	 *
	 */
	int SetRemoteWriterCryptoTokens(const GUID_t* writerId, const Sequence<CryptoToken>* tokens);


	/**
	 * @brief 根据remotePrefix找到匹配的远端域参与者加解密句柄。
	 *
	 * @param [in] remotePrefix 远端域参与者GuidPrefix。
	 */
	ParticipantCryptoHandle GetRemoteParticipantCryptoHandle(Spdp* spdp, const GuidPrefix_t* remotePrefix);

	/**
	 * @brief根据加载的属性填充builtinSecureData_的成员，主要是属性掩码。
	 *
	 */
	ReturnCode_t FillSecureData();
#endif // DDSSECURITY
	/**
	 * @brief 获取buffer指向的序列化缓冲区大小端信息。
	 *
	 * @param [in] buffer 报文序列化缓冲区。
	 *
	 * @return true表示使用小端，false表示大端编码。
	 */
	bool GetEndianess(SerializedBuffer* buffer);
	
	/**
	 * @brief 用户数据键值生成函数。
	 *
	 * @param [in]  data     用户报文指针。
	 * @param [out] handle   根据报文生成的带有键值的句柄。
	 * @param [in]  forceMd5 是否强制进行MD5转换标志。
	 *
	 * @return 生成成功返回true，否则返回false。
	 */
	bool MakeKey(const void*  data, InstanceHandle_t& handle, bool forceMd5) const;

	/**
	 * @brief 保存最后接收的样本实例。
	 *
	 * @param [in] cache 最后接收的样本实例。
	 */
	void SetLastinstanceHandle(const std::shared_ptr<CacheChange>& cache);

	/**
	 * @brief Qos策略中deadline Qos相关定时器超时执行函数。
	 */
	void DeadlineTimeout();

	/**
	 * @brief 检测新到达数据是否超期，超期则过滤。
	 *
	 * @param [in]  cache 新的用户报文。
	 *
	 * @return 检测未超期返回true，否则返回false。
	 */
	bool CheckLifespan(const std::shared_ptr<CacheChange>& cache);

	/**
	 * @brief 删除过期样本的方法，在lifespan Qos相关定时器超时执行函数。
	 */
	//bool LifespanTimeout();

	 /**
	  * @brief 更新活跃度更改情况。
	  *
	  * @param [in] handle   发布者句柄。
	  * @param [in] alive    节点存活状态。
	  * @param [in] ensure   明确节点失活（通知了），或者先前明确失活了，现又成功活了为  true，否则 false。
	  *
	  * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	ReturnCode_t UpdateLivelinessChangedStatus(const InstanceHandle_t& handle,const bool& alive,const bool& ensure = true);

	/**
	 * @brief 设置当前DataReader及其关联的订阅者状态。
	 *
	 * @param [in] add  指定的状态应设置为已触发还是未触发标志。
	 */
	void SetStatus(bool add);

	/**
	 * @brief 获取样本计数器。
	 *
	 * @param [in] trigger 触发器，用于触发ReadConditionImpl读取标志。
	 * 
	 * @return 样本计数器对象。
	 */
	SampleCounters GetSampleCounters(bool trigger = true);

	/**
	 * @brief 通知读取条件。
	 * 获取检索条件的触发值并唤醒附加到此通知程序的所有WaitSet实现。
	 */
	void NotifyReadConditions();

	/**
	 * @brief 获取所属的订阅者。
	 */
	SubscriberImpl* getParent();


	void ContentFilterUpdate();

	bool ValidateFilterData(SerializedBuffer* buffer);

private:
	std::vector<std::shared_ptr<ReadConditionImpl>> readcdts_;
	std::vector<std::shared_ptr<QueryConditionImpl>> querycdts_;
	TopicDescriptionImpl* topic_ = nullptr;
	DataReaderQos qos_;
	DataReaderListener* listener_ = nullptr;
	std::shared_ptr<Reader> reader_ = nullptr;
	SubscriberImpl* sub_ = nullptr;
	std::unique_ptr<DataReaderStatusManager> statusMgr_;
	std::shared_ptr<Timer> deadlineTimer_;
	/* 当前deadline计时器所属实例 */
	InstanceHandle_t lastInstance_;
	mutable std::mutex lastInstanceMtx_;
	/* wangyi lifespan 定时器 */
	// std::shared_ptr<Timer> lifespanTimer_;
	SubscriptionBuiltinTopicData builtinData_;
#ifdef DDSSECURITY
	SubscriptionBuiltinTopicSecureData builtinSecureData_;
#endif // DDSSECURITY
	ContentFilterProperty_t contentFilter_;
	std::shared_ptr<DDSSQL::DDSSQLFilter> filter_;
protected:
	TypeSupport* typeSupport_ = nullptr;
    std::shared_ptr<HistoryCache> history_;
#ifdef DDSSECURITY
public:
	/** @brief 指明DataReader是否启用了安全通信 */
	bool enableSec_ = false;
	/** @brief DataReader所对应的安全相关属性 */
	EndpointSecurityAttributes secAttributes_;
	/** @brief DataReader使用的加密/解密句柄 */
	DatareaderCryptoHandle cryptoHdl_ = NULL;
	/** @brief 子消息加密相关属性 get_datareader_submessage_encode_attributes获得*/
	CryptoReaderSubmessageAttributes submsgEncodeAttr_;
	/** @brief 安全插件实例 */
	SecurityPluginInstanceImpl* secPlugin_ = NULL;
	/** @brief DataWriter加密/解密句柄列表 */
	std::vector<DatawriterCryptoHandle> secWriters_;
	/** @brief 指明RTPS消息是否被加密 */
	bool rtpsMsgProtected_;

	/** @brief 启用了安全通信且未获得完整信息的DataWriter列表 */
	std::map<GUID_t, WriterSampleList*> preparingWriters_; //TOCHECK
	/** @brief   以DataWriter组织的样本链表. */
	std::map<GUID_t, WriterSampleList*> writerSampleList_;
	/** @brief 从GUID到DataWriter加密/解密句柄的映射 */
	std::map<GUID_t, Sequence<CryptoToken>> remoteWriterCryptoToken_;
	/** @brief 在所属域参与者启用安全下是否创建成功 */
	bool createSuccess_;
#endif // DDSSECURITY
};
TRAVODDS_NAMESPACE_END

#endif // !DATA_READER_IMPL_H
