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
#ifndef AGIDDS_RTPS_MESSAGES_PARAMETER_H
#define AGIDDS_RTPS_MESSAGES_PARAMETER_H

#include "type/basictype.h"
#include "qos/qos.h"
#include "rtps/discovery/discoverydata.h"

AGIDDS_NAMESPACE_BEGIN

enum ParameterId : uint16_t {
	/* rtps v2.5 Table 9.18*/
	PID_PAD                                 = 0x0000,
	PID_SENTINEL                            = 0x0001,
	PID_USER_DATA                           = 0x002c,
	PID_TOPIC_NAME                          = 0x0005,
	PID_TYPE_NAME                           = 0x0007,
	PID_GROUP_DATA                          = 0x002d,
	PID_TOPIC_DATA                          = 0x002e,
	PID_DURABILITY                          = 0x001d,
	PID_DURABILITY_SERVICE                  = 0x001e,
	PID_DEADLINE                            = 0x0023,
	PID_LATENCY_BUDGET                      = 0x0027,
	PID_LIVELINESS                          = 0x001b,
	PID_RELIABILITY                         = 0x001a,
	PID_LIFESPAN                            = 0x002b,
	PID_DESTINATION_ORDER                   = 0x0025,
	PID_HISTORY                             = 0x0040,
	PID_RESOURCE_LIMITS                     = 0x0041,
	PID_OWNERSHIP                           = 0x001f,
	PID_OWNERSHIP_STRENGTH                  = 0x0006,
	PID_PRESENTATION                        = 0x0021,
	PID_PARTITION                           = 0x0029,
	PID_TIME_BASED_FILTER                   = 0x0004,
	PID_TRANSPORT_PRIORITY                  = 0x0049,
	PID_DOMAIN_ID                           = 0x000f,   // new add
	PID_DOMAIN_TAG                          = 0x4014,   // new add
	PID_PROTOCOL_VERSION                    = 0x0015,
	PID_VENDORID                            = 0x0016,
	PID_UNICAST_LOCATOR                     = 0x002f,
	PID_MULTICAST_LOCATOR                   = 0x0030,
	PID_DEFAULT_UNICAST_LOCATOR             = 0x0031,
	PID_DEFAULT_MULTICAST_LOCATOR           = 0x0048,
	PID_METATRAFFIC_UNICAST_LOCATOR         = 0x0032,
	PID_METATRAFFIC_MULTICAST_LOCATOR       = 0x0033,
	PID_EXPECTS_INLINE_QOS                  = 0x0043,
	PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT = 0x0034,
	PID_PARTICIPANT_LEASE_DURATION          = 0x0002,
	PID_CONTENT_FILTER_PROPERTY             = 0x0035,
	PID_PARTICIPANT_GUID                    = 0x0050,
	PID_GROUP_GUID                          = 0x0052,
	PID_GROUP_ENTITY_ID                     = 0x0053,
	PID_BUILTIN_ENDPOINT_SET                = 0x0058,
	PID_BUILTIN_ENDPOINT_QOS                = 0x0077,
	PID_PROPERTY_LIST                       = 0x0059,
	PID_TYPE_MAX_SIZE_SERIALIZED            = 0x0060,
	PID_ENTITY_NAME                         = 0x0062,
	PID_ENDPOINT_GUID                       = 0x005a,
		
	//内联Qos rtps v2.5 Table 9.20
	PID_CONTENT_FILTER_INFO                 = 0x0055,    // new add
	PID_COHERENT_SET                        = 0x0056,    // new add
	PID_DIRECTED_WRITE                      = 0x0057,    // new add
	PID_ORIGINAL_WRITER_INFO                = 0x0061,    // new add
	PID_GROUP_COHERENT_SET                  = 0x0063,    // new add
	PID_GROUP_SEQ_NUM                       = 0x0064,    // new add
	PID_WRITER_GROUP_INFO                   = 0x0065,    // new add
	PID_SECURE_WRITER_GROUP_INFO            = 0x0066,    // new add
	PID_KEY_HASH                            = 0x0070,
	PID_STATUS_INFO                         = 0x0071,

	//Deprecated By Version 2.4
    //PID_MULTICAST_IPADDRESS                 = 0x0011,
    //PID_DEFAULT_UNICAST_IPADDRESS           = 0x000c,
    //PID_DEFAULT_UNICAST_PORT                = 0x000e,
    //PID_METATRAFFIC_UNICAST_IPADDRESS       = 0x0045,
    //PID_METATRAFFIC_UNICAST_PORT            = 0x000d,
    //PID_METATRAFFIC_MULTICAST_IPADDRESS     = 0x000b,
    //PID_METATRAFFIC_MULTICAST_PORT          = 0x0046,
	//PID_PARTICIPANT_BUILTIN_ENDPOINTS       = 0x0044,
	//PID_PARTICIPANT_ENTITYID                = 0x0051,
};

/**
 * @brief ParameterHeader类，RTPS报文参数标题类 。
 */
class ParameterHeader {
public:
	ParameterId pid_;
	uint16_t length_;

	/**
	 * @brief 构造函数。
	 */
	ParameterHeader()
		: pid_(ParameterId::PID_PAD),
		  length_(0)
	{
	}
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] pid 参数类型。
	 * @param [in] uint16_t 参数长度。
	 */
	ParameterHeader(ParameterId pid, uint16_t length)
		: pid_(pid),
		  length_(length)
	{
	}
	/**
	 * @brief 析构函数。
	 */
	virtual ~ParameterHeader() {};
	/**
	 * @brief 添加参数到报文中。
	 * allowLength  传入指针当前允许写入的字节长度,结果返回剩余可写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in,out] allowLength 报文指针偏移量。
	 *
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual bool AddToMessage(octet* msg, uint16_t& allowLength) = 0;
	/**
	 * @brief 获取参数长度。
	 * 返回的不一定是 length_的值，除非length_的值是4的整数倍值 ,否则是length_的下一个4的整数倍值。
	 *
	 * @return 参数长度值。
	 */
	virtual uint16_t MessageSize() const {
		return (length_ + 3u) & ~3u;
	};

	uint16_t StringSize(const std::string& s ) const {
		size_t len = s.size();
		// 检查最后一个字符是否是 '\0'（可选）
		bool has_null = !s.empty() && s.back() != '\0';
		return len + (has_null ? 1 : 0);
	}
};

/**
 * @brief Parameter模板类，RTPS报文参数类。
 * 实现对报文已知类型参数的封装。
 */
template<typename T>
class Parameter : public ParameterHeader {
public:
	/**
	 * @brief 构造函数。
	 *  note:若是没有对应的映射T，传入T = std::string 即可
	 *
	 * @param [in] pid 参数类型。
	 * @param [in] parameter 参数值。
	 * @param [in] length 参数长度（默认值：0 ）。
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	Parameter(const ParameterId& pid, const T& parameter, const uint16_t& length = 0,const EndiannessKind& endian = PL_DEFAULT_ENDIAN)
		: ParameterHeader(pid, length),
		  parameter_(parameter),
		  msgEndian_(endian)
    {
		length_ = size();
	}

	/**
	 * @brief 析构函数。
	 */
	virtual  ~Parameter() {
	}

	/**
	 * @brief 获取参数对象。
	 *
	 * @param [out] parameter 参数对象。
	 */
	void GetParameter(T& parameter) {
		parameter = parameter_;
	};

	/**
	 * @brief 获取参数对象。
	 *
	 * @return 参数对象变量parameter_值。
	 */
	T& GetParameter() {
	   return parameter_;
	};

	/**
	 * @brief 将参数头封装到报文中。
	 *
	 * @param [in] msg 报文指针。
	 * @param [in,out] allowLength 报文指针偏移量。
	 */
	bool AddToMessage(octet* msg, uint16_t& allowLength) {
		uint16_t len = MessageSize();

		if (allowLength < 4 + len) {
			return false;
		}
		/*  size of (pid +（数据长度+补位)) = 4 */
		WriterUInt16(msg, static_cast<uint16_t>(pid_));
		WriterUInt16(msg + 2, len);

		writerMsg(msg + 4);

		allowLength -= 4 + len;
		return true;
	}
	/**
	 * @brief 获取参数长度。
	 * 返回的不一定是 length_的值，除非length_的值是4的整数倍值 ,否则是length_的下一个4的整数倍值。
	 *
	 * @return 参数长度。
	 */
	virtual uint16_t MessageSize() const {
		return (length_ + 3u) & ~3u;
	};
public:
	/**
	 * @brief 将参数封装到报文中。
	 *
	 * @param [in,out] msg 报文指针。
	 */
	void writerMsg(octet* msg) {
		return;
	}
	/**
	 * @brief 获取参数长度。
	 * 返回的不一定是 length_的值，除非length_的值是4的整数倍值 ,否则是length_的下一个4的整数倍值。
	 *
	 * @return 参数长度值。
	 */
	uint16_t size() {
		return  length_;
	}

protected:
	/**
	 * @brief int16_t类型参数值封装函数。
	 * 返回的是写入的长度
	 * 
	 * @param [out] msg 报文指针。
	 * @param [in] i16 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterInt16(octet* msg, const int16_t& i16);
	/**
	 * @brief uint16_t类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] ui16 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterUInt16(octet* msg, const uint16_t& ui16);
	/**
	 * @brief int32_t类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] i32 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterInt32(octet* msg, const int32_t& i32);
	/**
	 * @brief uint32_t类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] ui32 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterUInt32(octet* msg, const uint32_t& ui32);
	/**
	 * @brief octet*类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] src 参数值。
	 * @param [in] len 参数长度。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterInt(octet* msg, const octet* src, const octet& len);
	/**
	 * @brief string类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] str 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterString(octet* msg, const std::string& str);
	/**
	 * @brief Locator_t类型参数值封装函数。
	 * 返回的是写入的长度
	 *
	 * @param [out] msg 报文指针。
	 * @param [in] loc 参数值。
	 *
	 * @return 参数长度值。
	 */
	uint32_t WriterLocator(octet* msg, const Locator_t& loc);
private:
	T parameter_;
	EndiannessKind msgEndian_;
};
//octet[2]
template <>
inline Parameter<VendorId_t>::Parameter(const ParameterId& pid,const VendorId_t& parameter, const uint16_t& length, const EndiannessKind& endian)
	: ParameterHeader(pid, length), msgEndian_(endian)
{
	parameter_[0] = parameter[0];
	parameter_[1] = parameter[1];
	length_ = sizeof(VendorId_t);
}
template <>
inline void Parameter<VendorId_t>::GetParameter(VendorId_t& parameter)
{
	parameter[0] = parameter_[0];
	parameter[1] = parameter_[1];
}
//octet[4]
template <>
inline Parameter<StatusInfo_t>::Parameter(const ParameterId& pid, const StatusInfo_t& parameter, const uint16_t& length, const EndiannessKind& endian)
	: ParameterHeader(pid, length), msgEndian_(endian)
{
	length_ = sizeof(StatusInfo_t);
	memcpy(parameter_, parameter, length_);
}
template <>
inline void Parameter<StatusInfo_t>::GetParameter(StatusInfo_t& parameter)
{
	memcpy(parameter, parameter_, length_);
}
//octet[16]
template <>
inline Parameter<KeyHash_t>::Parameter(const ParameterId& pid, const KeyHash_t& parameter, const uint16_t& length, const EndiannessKind& endian)
	: ParameterHeader(pid, length), msgEndian_(endian)
{
	length_ = sizeof(KeyHash_t);
	memcpy(parameter_, parameter, length_);
}
template <>
inline void Parameter<KeyHash_t>::GetParameter(KeyHash_t& parameter)
{
	memcpy(parameter, parameter_, length_);
}

template<typename T>
inline uint32_t Parameter<T>::WriterInt16(octet* msg, const int16_t& i16) 
{
	octet* src = (octet*)&i16;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(msg, src, 2);
	}
	else {
		msg[0] = src[1];
		msg[1] = src[0];
	}
	return 2;
}

template<typename T>
inline uint32_t Parameter<T>::WriterUInt16(octet* msg, const uint16_t& ui16) 
{
	octet* src = (octet*)&ui16;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(msg, src, 2);
	} else {
		msg[0] = src[1];
		msg[1] = src[0];
	}
	return 2;
}

template<typename T>
inline uint32_t Parameter<T>::WriterInt32(octet* msg, const int32_t& i32) 
{
	octet* src = (octet*)&i32;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(msg, src, 4);
	} else {
		msg[0] = src[3];
		msg[1] = src[2];
		msg[2] = src[1];
		msg[3] = src[0];
	}
	return 4;
}

template<typename T>
inline uint32_t Parameter<T>::WriterUInt32(octet* msg, const uint32_t& ui32) 
{
	octet* src = (octet*)&ui32;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(msg, src, 4);
	} else {
		msg[0] = src[3];
		msg[1] = src[2];
		msg[2] = src[1];
		msg[3] = src[0];
	}
	return 4;
}

template<typename T>
inline uint32_t Parameter<T>::WriterInt(octet* msg, const octet* src, const octet& len) 
{
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(msg, src, len);
	} else {
		for (int32_t i = 0; i < len; i++) {
			msg[i] = src[len - 1 - i];
		}
	}
	return len;
}


template<typename T>
inline uint32_t Parameter<T>::WriterString(octet* msg, const std::string& str) 
{
	/* wangyi fastdds适配 */
	uint32_t size = StringSize(str);
	WriterUInt32(msg, size);
	if (size != 0) {
		memcpy(msg + 4, str.data(), str.size());
		memset(msg + 4 + str.size(), 0,1);
	}
	return 4u + (size + 3u) & ~3u;
}

template<typename T>
inline uint32_t Parameter<T>::WriterLocator(octet* msg, const Locator_t& loc) 
{
	WriterInt32(msg, loc.kind);
	WriterUInt32(msg + 4, loc.port);
	memcpy(msg + 8, loc.address, 16);
	return 24u;
}
//bool
template <>
inline uint16_t Parameter<bool>::size() 
{
	return 1;
}
template <>
inline void Parameter<bool>::writerMsg(octet* msg) 
{
	*msg = static_cast<octet>(parameter_);
}
//std::string
template <>
inline Parameter<std::string>::Parameter(const ParameterId& pid, const std::string& parameter, const uint16_t& length, const EndiannessKind& endian)
	: ParameterHeader(pid, length), msgEndian_(endian)
{
	parameter_ = parameter;
	length_ = ((StringSize(parameter_) + 3u) & ~3u) + 4;
}
template <>
inline void Parameter<std::string>::writerMsg(octet* msg) 
{
	WriterString(msg, parameter_);
}
//int32_t
template <>
inline uint16_t Parameter<int32_t>::size() 
{
	return 4;
}
template <>
inline void Parameter<int32_t>::writerMsg(octet* msg) 
{
	WriterInt32(msg, parameter_);
}
//uint32_t
template <>
inline uint16_t Parameter<uint32_t>::size() 
{
	return 4;
}
template <>
inline void Parameter<uint32_t>::writerMsg(octet* msg) 
{
	WriterUInt32(msg, parameter_);
}
//VendorId_t
template <>
inline uint16_t Parameter<VendorId_t>::size() 
{
	return 2;
}
template <>
inline void Parameter<VendorId_t>::writerMsg(octet* msg) 
{
	memcpy(msg, parameter_.data(), 2);
}
//LocatorSeq, 特殊每一个Locator都具有单独的 pid+len = 4，兼容做特殊处理
template <>
inline uint16_t Parameter<LocatorSeq>::size() 
{
	length_ = parameter_.size() * (4 + 24) - 4;
	return length_;
}
template <>
inline bool Parameter<LocatorSeq>::AddToMessage(octet* msg, uint16_t& allowLength) 
{
	uint16_t len = MessageSize();

	if (allowLength < 4 + len) {
		return false;
	}
	uint32_t offset = 0;
	for (auto& it : parameter_) {
		WriterUInt16(msg + offset, static_cast<uint16_t>(pid_));
		WriterUInt16(msg + offset + 2, 24);
		WriterInt32(msg + offset + 4, it.kind);
		WriterUInt32(msg + offset + 8, it.port);
		memcpy(msg + offset + 12, it.address, 16);
		offset += 28;
	}
	allowLength -= 4 + len;
	return true;
}
//Duration_t
template <>
inline uint16_t Parameter<Duration_t>::size() 
{
	return 8;
}
template <>
inline void Parameter<Duration_t>::writerMsg(octet* msg) 
{
	WriterInt32(msg, parameter_.sec);
	WriterUInt32(msg+4, parameter_.nanosec);
}
//BuiltinTopicKey_t
template <>
inline uint16_t Parameter<BuiltinTopicKey_t>::size() 
{
	return 16;
}
template <>
inline void Parameter<BuiltinTopicKey_t>::writerMsg(octet* msg) 
{
	memcpy(msg, parameter_.value, 16);
}

// UserDataQosPolicy GroupDataQosPolicy TopicDataQosPolicy
#define IMPLEMENT_PARAMETER_SEQ_SERIALIZATION(QosPolicy) \
template <> \
inline uint16_t Parameter<QosPolicy>::size() { \
    return 4 + static_cast<uint16_t>(parameter_.value.size()); \
} \
template <> \
inline void Parameter<QosPolicy>::writerMsg(octet* msg) { \
    uint32_t size = parameter_.value.size(); \
    WriterUInt32(msg, size); \
    memcpy(msg + 4, parameter_.value.data(), size); \
}

IMPLEMENT_PARAMETER_SEQ_SERIALIZATION(UserDataQosPolicy)
IMPLEMENT_PARAMETER_SEQ_SERIALIZATION(GroupDataQosPolicy)
IMPLEMENT_PARAMETER_SEQ_SERIALIZATION(TopicDataQosPolicy)

//DurabilityQosPolicy DestinationOrderQosPolicy OwnershipQosPolicy
#define IMPLEMENT_PARAMETER_KIND_SERIALIZATION(QosPolicy) \
template <> \
inline uint16_t Parameter<QosPolicy>::size() { \
	return 1; \
} \
template <> \
inline void Parameter<QosPolicy>::writerMsg(octet* msg) { \
	*msg = static_cast<octet>(parameter_.kind); \
}
IMPLEMENT_PARAMETER_KIND_SERIALIZATION(DurabilityQosPolicy)
IMPLEMENT_PARAMETER_KIND_SERIALIZATION(DestinationOrderQosPolicy)
IMPLEMENT_PARAMETER_KIND_SERIALIZATION(OwnershipQosPolicy)

//DurabilityServiceQosPolicy
template <>
inline uint16_t Parameter<DurabilityServiceQosPolicy>::size() 
{
	return 8 + 4 + 16;
}
template <>
inline void Parameter<DurabilityServiceQosPolicy>::writerMsg(octet* msg) 
{
	WriterInt32(msg, parameter_.service_cleanup_delay.sec);
	WriterUInt32(msg + 4, parameter_.service_cleanup_delay.nanosec);
	*(msg+8) = static_cast<octet>(parameter_.history_kind);
	WriterInt32(msg + 12, parameter_.history_depth);
	WriterInt32(msg + 16, parameter_.max_samples);
	WriterInt32(msg + 20, parameter_.max_instances);
	WriterInt32(msg + 24, parameter_.max_samples_per_instance);
}
//DeadlineQosPolicy
template <>
inline uint16_t Parameter<DeadlineQosPolicy>::size() 
{
	return 8;
}
template <>
inline void Parameter<DeadlineQosPolicy>::writerMsg(octet* msg) 
{
	WriterInt32(msg, parameter_.period.sec);
	WriterUInt32(msg + 4, parameter_.period.nanosec);
}
//LatencyBudgetQosPolicy LifespanQosPolicy
#define IMPLEMENT_PARAMETER_DURATION_SERIALIZATION(QosPolicy) \
template <> \
inline uint16_t Parameter<QosPolicy>::size() { \
	return 8; \
} \
template <> \
inline void Parameter<QosPolicy>::writerMsg(octet* msg) { \
	WriterInt32(msg, parameter_.duration.sec); \
	WriterUInt32(msg + 4, parameter_.duration.nanosec); \
}
IMPLEMENT_PARAMETER_DURATION_SERIALIZATION(LatencyBudgetQosPolicy)
IMPLEMENT_PARAMETER_DURATION_SERIALIZATION(LifespanQosPolicy)

//LivelinessQosPolicy
template <>
inline uint16_t Parameter<LivelinessQosPolicy>::size() 
{
	return 12;
}
template <>
inline void Parameter<LivelinessQosPolicy>::writerMsg(octet* msg) 
{
	*(msg) = static_cast<octet>(parameter_.kind);
	WriterInt32(msg + 4, parameter_.lease_duration.sec);
	WriterUInt32(msg + 8, parameter_.lease_duration.nanosec);
}
//ReliabilityQosPolicy
template <>
inline uint16_t Parameter<ReliabilityQosPolicy>::size() 
{
	return 12;
}
template <>
inline void Parameter<ReliabilityQosPolicy>::writerMsg(octet* msg) 
{
	*(msg) = static_cast<octet>(parameter_.kind);
	WriterInt32(msg + 4, parameter_.max_blocking_time.sec);
	WriterUInt32(msg + 8, parameter_.max_blocking_time.nanosec);
}
//OwnershipStrengthQosPolicy
template <>
inline uint16_t Parameter<OwnershipStrengthQosPolicy>::size() 
{
	return 4;
}
template <>
inline void Parameter<OwnershipStrengthQosPolicy>::writerMsg(octet* msg) 
{
	WriterInt32(msg, parameter_.value);
}
//PresentationQosPolicy
template <>
inline uint16_t Parameter<PresentationQosPolicy>::size() 
{
	return 6;
}
template <>
inline void Parameter<PresentationQosPolicy>::writerMsg(octet* msg) 
{
	*(msg) = static_cast<octet>(parameter_.access_scope);
	*(msg + 4) = static_cast<octet>(parameter_.coherent_access);
	*(msg + 5) = static_cast<octet>(parameter_.ordered_access);
}
//PartitionQosPolicy
template <>
inline uint16_t Parameter<PartitionQosPolicy>::size() 
{
	uint32_t inSize = 0;
	for (uint32_t i = 0; i < parameter_.name.size(); i++) {
		inSize += (4u + ((StringSize(parameter_.name[i]) + 3u) & ~3u));
	}
	return 4 + inSize;
}
template <>
inline void Parameter<PartitionQosPolicy>::writerMsg(octet* msg) 
{
	uint32_t num = parameter_.name.size();
	WriterUInt32(msg,num);
	uint32_t inOffset = 4;
	for (uint32_t i = 0; i < num; i++) {
		inOffset += WriterString(msg + inOffset, parameter_.name[i]);
	}
}
//GUID_t
template <>
inline uint16_t Parameter<GUID_t>::size() 
{
	return 16;
}
template <>
inline void Parameter<GUID_t>::writerMsg(octet* msg) 
{
	memcpy(msg,&parameter_,16);
}
//EntityId_t
template <>
inline uint16_t Parameter<EntityId_t>::size()
{
	return 4;
}
template <>
inline void Parameter<EntityId_t>::writerMsg(octet* msg) 
{
	memcpy(msg, &parameter_, 4);
}
//TimeBasedFilterQosPolicy
template <>
inline uint16_t Parameter<TimeBasedFilterQosPolicy>::size()
{
	return 8;
}
template <>
inline void Parameter<TimeBasedFilterQosPolicy>::writerMsg(octet* msg)
{
	WriterInt32(msg, parameter_.minimum_separation.sec);
	WriterUInt32(msg + 4, parameter_.minimum_separation.nanosec);
}
//ContentFilterProperty_t
template <>
inline uint16_t Parameter<ContentFilterProperty_t>::size()
{
	uint32_t inSize = 0;
	inSize += 4 + ((StringSize(parameter_.contentFilteredTopicName) + 3u) & ~3u);
	inSize += 4 + ((StringSize(parameter_.relatedTopicName) + 3u) & ~3u);
	inSize += 4 + ((StringSize(parameter_.filterClassName) + 3u) & ~3u);
	inSize += 4 + ((StringSize(parameter_.filterExpression) + 3u) & ~3u);

	for (uint32_t i = 0; i < parameter_.expressionParameters.size(); i++) {
		inSize += (4u + ((StringSize(parameter_.expressionParameters[i]) + 3u) & ~3u));
	}
	return 4 + inSize;
}
template <>
inline void Parameter<ContentFilterProperty_t>::writerMsg(octet* msg)
{
	uint32_t inOffset = 0;
	inOffset += WriterString(msg, parameter_.contentFilteredTopicName);
	inOffset += WriterString(msg + inOffset, parameter_.contentFilteredTopicName);
	inOffset += WriterString(msg + inOffset, parameter_.relatedTopicName);
	inOffset += WriterString(msg + inOffset, parameter_.filterClassName);
	inOffset += WriterString(msg + inOffset, parameter_.filterExpression);

	uint32_t num = parameter_.expressionParameters.size();
	WriterUInt32(msg + inOffset, num);
	inOffset += 4;
	for (uint32_t i = 0; i < num; i++) {
		inOffset += WriterString(msg + inOffset, parameter_.expressionParameters[i]);
	}
}
//HistoryQosPolicy
template <>
inline uint16_t Parameter<HistoryQosPolicy>::size()
{
	return 8;
}
template <>
inline void Parameter<HistoryQosPolicy>::writerMsg(octet* msg)
{
	*msg = static_cast<octet>(parameter_.kind);
	WriterInt32(msg + 4, parameter_.depth);
}
//ResourceLimitsQosPolicy
template <>
inline uint16_t Parameter<ResourceLimitsQosPolicy>::size()
{
	return 12;
}
template <>
inline void Parameter<ResourceLimitsQosPolicy>::writerMsg(octet* msg)
{
	WriterInt32(msg, parameter_.max_samples);
	WriterInt32(msg + 4, parameter_.max_instances);
	WriterInt32(msg + 8, parameter_.max_samples_per_instance);
}
//SequenceNumber_t
template <>
inline uint16_t Parameter<SequenceNumber_t>::size()
{
	return 8;
}
template <>
inline void Parameter<SequenceNumber_t>::writerMsg(octet* msg)
{
	WriterInt32(msg, parameter_.high);
	WriterUInt32(msg + 4, parameter_.low);
}
// PropertySeq
template <>
inline uint16_t Parameter<PropertySeq>::size()
{
	uint32_t inSize = 0;
	inSize += 4u;
	for (const auto& prop : parameter_) {
		inSize += 4u + ((StringSize(prop.name) + 3u) & ~3u);
		inSize += 4u + ((StringSize(prop.value) + 3u) & ~3u);
	}
	return inSize;
}
template <>
inline void Parameter<PropertySeq>::writerMsg(octet* msg)
{
	uint32_t inOffset = 0;
	inOffset += WriterUInt32(msg, parameter_.size());
	for (const auto& prop : parameter_) {
		inOffset += WriterString(msg + inOffset, prop.name);
		inOffset += WriterString(msg + inOffset, prop.value);
	}
}
//ContentFilterInfo_t
template <>
inline uint16_t Parameter<ContentFilterInfo_t>::size()
{
	uint32_t inSize = 4;
	inSize += 4 * parameter_.filterResult.size();
	inSize += 4;
	inSize += 16 * parameter_.filterSignatures.size();
	return inSize;
}
template <>
inline void Parameter<ContentFilterInfo_t>::writerMsg(octet* msg)
{
	uint32_t num = parameter_.filterResult.size();
	uint32_t inOffset = 0;
	inOffset += WriterUInt32(msg, num);
	for (uint32_t i = 0; i < num; i++) {
		inOffset += WriterInt32(msg + inOffset, parameter_.filterResult[i]);
	}
	num = parameter_.filterSignatures.size();
	inOffset += WriterUInt32(msg, num);
	for (uint32_t i = 0; i < num; i++) {
		inOffset += WriterInt32(msg + inOffset, parameter_.filterSignatures[i][0]);
		inOffset += WriterInt32(msg + inOffset, parameter_.filterSignatures[i][1]);
		inOffset += WriterInt32(msg + inOffset, parameter_.filterSignatures[i][2]);
		inOffset += WriterInt32(msg + inOffset, parameter_.filterSignatures[i][3]);
	}
}

//OriginalWriterInfo_t
template <>
inline uint16_t Parameter<OriginalWriterInfo_t>::size()
{
	uint32_t inSize = 24;
	uint32_t num = parameter_.originalWriterQos.size();
	for (uint32_t i = 0; i < num; i++) {
		inSize += 4 + (parameter_.originalWriterQos[i].length + 3u) & ~3u;
	}
	return inSize + 4u;
}
template <>
inline void Parameter<OriginalWriterInfo_t>::writerMsg(octet* msg)
{
	memcpy(msg, &parameter_.originalWriterGUID, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	WriterInt32(msg + 16, parameter_.originalWriterSN.high);
	WriterUInt32(msg + 20, parameter_.originalWriterSN.low);
	uint32_t num = parameter_.originalWriterQos.size();
	uint32_t inOffset = 24;
	for (uint32_t i = 0; i < num; i++) {
		WriterUInt16(msg + inOffset, parameter_.originalWriterQos[i].parameterId);
		WriterUInt16(msg + inOffset + 2, parameter_.originalWriterQos[i].length);
		memcpy(msg + inOffset + 4, parameter_.originalWriterQos[i].value.data(), parameter_.originalWriterQos[i].length);

		inOffset += 4 + (parameter_.originalWriterQos[i].length + 3u) & ~3u;
	}

	unsigned char sentinel[2] = { 0x0,0x1 };
	if (msgEndian_ == EndiannessKind::PL_CDR_BE) {
		(msg + inOffset)[0] = sentinel[0];
		(msg + inOffset)[1] = sentinel[1];
	} else {
		(msg + inOffset)[0] = sentinel[1];
		(msg + inOffset)[1] = sentinel[0];
	}
}
//WriterGroupInfo_t
template <>
inline uint16_t Parameter<WriterGroupInfo_t>::size()
{
	return 4;
}
template <>
inline void Parameter<WriterGroupInfo_t>::writerMsg(octet* msg)
{
	memcpy(msg, parameter_.writerSet, 4);
}

//ProtocolVersion_t
template <>
inline unsigned short Parameter<ProtocolVersion_t>::size()
{
	return 2;
}
template <>
inline void Parameter<ProtocolVersion_t>::writerMsg(octet* msg)
{
	memcpy(msg, &parameter_, 2);
}

/**
 * @brief ParameterList类，RTPS报文参数Parameter集合管理类。
 *  rtps v2.5 Table 9.19
 */
class ParameterList {
public:
	/**
	 * @brief 默认构造函数。
	 *  
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	ParameterList(const EndiannessKind& endian = PL_DEFAULT_ENDIAN) : msgEndian_(endian) {};
	/**
	 * @brief ParameterList类通用构造函数。
	 *  msg 包括cdr大小端信息时报文结构。
     * 0.......... 8........... 16........ 24.......... 32
	 * +-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+-+-+-+-+-+-+ + 
     *| 0x00        PL_CDR_LE  | 0x00         0x00      |  cdr大小端信息
     * +-+-+-+-+-+-+-+-+-+-+--+-+-+-+-+-+-+-+-+-+-+-+-+ +
     *| ParameterId            | len                    |
     * +---------- + --------- + ---------- + --------- +
     *| octet[len]	                                    |
     * +---------- + --------- + ------ --- + --------- +
	 *| ParameterId            | len                    |
	 * +---------- + --------- + ---------- + --------- +
	 *| octet[len]	                                    |
	 * +---------- + --------- + ------ --- + --------- +
	 * PID_SENTINEL（终止符）   |                        |
	 * +---------- + --------- + ------ --- + --------- +
	 * 
	 * @param [in] msg 上述Parameter结构报文。
	 * @param [in] length 报文长度。
	 * 
     */
	ParameterList(const octet* msg, const uint32_t& length);
	/**
	 * @brief SPDPdiscoveredParticipantData结构对应构造函数。
	 *
	 * @param [in] data SPDPdiscoveredParticipantData结构对象。
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	ParameterList(const SPDPdiscoveredParticipantData& data, const EndiannessKind& endian = PL_DEFAULT_ENDIAN);
	/**
	 * @brief DiscoveredWriterData结构对应构造函数。
	 *
	 * @param [in] data DiscoveredWriterData结构对象。
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	ParameterList(const DiscoveredWriterData& data, const EndiannessKind& endian = PL_DEFAULT_ENDIAN);
	/**
	 * @brief DiscoveredReaderData结构对应构造函数。
	 *
	 * @param [in] data DiscoveredReaderData结构对象。
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	ParameterList(const DiscoveredReaderData& data, const EndiannessKind& endian = PL_DEFAULT_ENDIAN);
	/**
	 * @brief DiscoveredTopicData结构对应构造函数。
	 *
	 * @param [in] data DiscoveredTopicData结构对象。
	 * @param [in] endian 大小端字节序标志（默认值：DEFAULT_ENDIAN）。
	 */
	ParameterList(const DiscoveredTopicData& data, const EndiannessKind& endian = PL_DEFAULT_ENDIAN);

	/**
	 * @brief 将Parameter集合数据转换为SPDPdiscoveredParticipantData结构数据。
	 *
	 * @param [out] data SPDPdiscoveredParticipantData结构对象。
	 */
	void GetDiscoveredData(SPDPdiscoveredParticipantData& data);
	/**
	 * @brief 将Parameter集合数据转换为DiscoveredWriterData结构数据。
	 *
	 * @param [out] data DiscoveredWriterData结构对象。
	 */
	void GetDiscoveredData(DiscoveredWriterData& data);
	/**
	 * @brief 将Parameter集合数据转换为DiscoveredReaderData结构数据。
	 *
	 * @param [out] data DiscoveredReaderData结构对象。
	 */
	void GetDiscoveredData(DiscoveredReaderData& data);
	/**
	 * @brief 将Parameter集合数据转换为DiscoveredTopicData结构数据。
	 *
	 * @param [out] data DiscoveredTopicData结构对象。
	 */
	void GetDiscoveredData(DiscoveredTopicData& data);

	/**
	 * @brief 获取参数集合总长度
	 * 
	 * @return 参数总长度。
	 */
	uint32_t GetMessageSize();
	/**
	 * @brief 序列化封装ParameterList中包含的Parameter结构参数到msg报文内存中。
	 *  msg 包括cdr大小端信息
	 *
	 * @param [out] msg 报文内存指针。
	 * @param [out] allowLength 偏移量。
	 * 
	 * @return 封装成功返回true,否则返回false。
	 */
	bool WriterToMessage(octet* msg, uint16_t& allowLength);

	/**
	 * @brief 反序列化解析msg报文为Parameter参数并存储到ParameterList容器中。
	 *  msg 不包括cdr大小端信息
	 *
	 * @param [in] msg 报文内存指针。
	 * @param [in] allowLength 偏移量。
	 *
	 * @return 解析成功返回true,否则返回false。
	 */
	void LoadMessage(const octet* msg, const uint32_t& length);
	
	/**
	 * @brief 将Parameter参数存储到ParameterList容器中。
	 *  任意类型都支持以Parameter_t结构存储,在容器内映射真实的结构。
	 *
	 * @param [in] pid Parameter参数类型。
	 * @param [in] parameter Parameter参数值。
	 * @param [in] length 参数长度。
	 */
	template <typename T>
	void AddParameter(const ParameterId& pid, const T& parameter, uint16_t length = 0) {
		parameterList_.insert({ pid,std::make_unique<Parameter<T>>(pid, parameter, length, msgEndian_) });
	}

	/**
	 * @brief 根据Parameter参数类型从ParameterList容器中获取对应的Parameter参数值。
	 *  任意类型都支持以Parameter_t结构读取。
	 *
	 * @param [in] pid Parameter参数类型。
	 * @param [out] parameter Parameter参数值。
	 */
	template <typename T>
	bool GetParameter(const ParameterId& pid, T& parameter) const {
		auto it = parameterList_.find(pid);
		Parameter<T>* typedParam = nullptr; ;

		if (it != parameterList_.end() &&
			nullptr != (typedParam = dynamic_cast<Parameter<T>*>(it->second.get()))) {
			typedParam->GetParameter(parameter);
			return true;
		}
		return false;
	}

	/**
	 * @brief 深拷贝parameterList_到一个新的ParameterList_t容器中。
	 *
	 * @param [out] list ParameterList_t类型容器。
	 * 
	 * @return list存储的Parameter参数个数。
	 */
	uint32_t GetParameterList(ParameterList_t& list);
protected:
	/**
	 * @brief 从msg报文中获取int16_t格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] i16 Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReadInt16(const octet* msg, int16_t& i16);
	/**
	 * @brief 从msg报文中获取int16_t格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] i16 Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReadUInt16(const octet* msg, uint16_t& ui16);
	/**
	 * @brief 从msg报文中获取int16_t格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] i16 Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReadInt32(const octet* msg, int32_t& i32);
	/**
	 * @brief 从msg报文中获取uint32_t格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] ui32 Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReadUInt32(const octet* msg, uint32_t& ui32);
	/**
	 * @brief 从msg报文中获取octet*格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] dest Parameter参数值。
	 * @param [in] len Parameter参数值长度。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReadInt(const octet* msg, octet* dest, const octet len);
	/**
	 * @brief 从msg报文中获取string格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] str Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReaderString(const octet* msg, std::string& str);
	/**
	 * @brief 从msg报文中获取Locator_t格式的Parameter参数。
	 *
	 * @param [in] msg 类型容器。
	 * @param [out] loc Parameter参数值。
	 *
	 * @return 读取的长度。
	 */
	uint32_t ReaderLocator(const octet* msg, Locator_t& loc);

private:
	std::map<ParameterId,std::unique_ptr<ParameterHeader>> parameterList_;
	EndiannessKind msgEndian_;
};

template <>
inline void ParameterList::AddParameter<Locator_t>(const ParameterId& pid, const Locator_t& parameter, uint16_t length) {
	auto it = parameterList_.find(pid);
	if (it == parameterList_.end()) {
		LocatorSeq seq;
		seq.push_back(parameter);
		parameterList_.insert({ pid,std::make_unique<Parameter<LocatorSeq>>(pid, seq, length, msgEndian_) });
	} else {
		auto typedParam = dynamic_cast<Parameter<LocatorSeq>*>(it->second.get());
		LocatorSeq& seq = typedParam->GetParameter();
		seq.push_back(parameter);
		typedParam->size();//更新length_值；
	}
}
template <>
inline void ParameterList::AddParameter<Parameter_t>(const ParameterId& pid, const Parameter_t& parameter, uint16_t length) {
	std::string data;
	data.resize(4 + parameter.value.size());
	octet* src = (octet*)&parameter;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(&data[0], src, 4);
	} else {
		data[0] = src[1];
		data[1] = src[0];
		data[2] = src[3];
		data[3] = src[2];
	}
	memcpy(&data[4], parameter.value.data(), parameter.length);
	LoadMessage((octet*)data.data(), data.size());
}
template <>
inline bool ParameterList::GetParameter(const ParameterId& pid, Parameter_t& parameter) const {
	auto it = parameterList_.find(pid);
	ParameterHeader* typedParam = nullptr; ;

	if (it != parameterList_.end() &&
		nullptr != (typedParam = it->second.get())) {
		parameter.parameterId = static_cast<ParameterId_t>(typedParam->pid_);
		parameter.length = typedParam->length_;
		parameter.value.resize(parameter.length);
		uint16_t size = typedParam->MessageSize() + 4;

		std::string msg;
		msg.resize(size);
		typedParam->AddToMessage((octet*)&msg[0], size);
		memcpy(parameter.value.data(), &msg[4], parameter.length);
		return true;
	}
	return false;
}

AGIDDS_NAMESPACE_END

#endif
