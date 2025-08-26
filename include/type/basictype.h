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
#ifndef TRAVODDS_TYPE_BASICTYPE_H
#define TRAVODDS_TYPE_BASICTYPE_H

#include <cstdint>
#include <array>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <string.h>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include "travodds_dll.h"
#include "numericconstants.h"
#include "common/sequence/sequence.h"
#include "dcps/subscribe/states.h"
#include "assert.h"


#define PRINTF_ERROR() printf("fun[%s] line[%d]\n",__func__, __LINE__);


TRAVODDS_NAMESPACE_BEGIN


/* 待实现的数据类型 */
#define DOMAINID_TYPE_NATIVE 	uint32_t
#define HANDLE_TYPE_NATIVE		int32_t
#define HANDLE_NIL_NATIVE 		0
#define BUILTIN_TOPIC_KEY_TYPE_NATIVE 	uint32_t

//#define TheParticipantFactory
//#define PARTICIPANT_QOS_DEFAULT
//#define TOPIC_QOS_DEFAULT
//#define PUBLISHER_QOS_DEFAULT
//#define SUBSCRIBER_QOS_DEFAULT
//#define DATAWRITER_QOS_DEFAULT
//#define DATAREADER_QOS_DEFAULT
#define DATAWRITER_QOS_USE_TOPIC_QOS
#define DATAREADER_QOS_USE_TOPIC_QOS

template <typename T>
using Sequence = std::vector<T>;

using DomainId_t = DOMAINID_TYPE_NATIVE;
const DomainId_t DOMAIN_ID_UNKNOWN = 0xFFFFFFFF;

#define MIG_RTPS_KEY_HASH_MAX_LENGTH 16

struct MIGRtpsKeyHash {
	unsigned char value[MIG_RTPS_KEY_HASH_MAX_LENGTH];

	uint32_t length;

	MIGRtpsKeyHash() {
		memset(value, 0, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	}

	MIGRtpsKeyHash(const MIGRtpsKeyHash& other) : length(other.length), value{ 0 } {

		memcpy(this->value, other.value, this->length);
	}

	bool operator == (const MIGRtpsKeyHash& right) const {
		if (this->length != right.length) {
			return false;
		}

		for (uint32_t i = 0; i < this->length; i++) {
			if (this->value[i] != right.value[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator != (const MIGRtpsKeyHash& right) const {
		if (this->length != right.length) {
			return true;
		}
		for (uint32_t i = 0; i < this->length; i++) {
			if (this->value[i] != right.value[i]) {
				return true;
			}
		}
		return false;
	}
	bool operator > (const MIGRtpsKeyHash& right) const {
		if (this->length != right.length) {
			return false;
		}
		for (uint32_t i = 0; i < this->length; i++) {
			if (this->value[i] > right.value[i]) {
				return true;
			} else if (this->value[i] == right.value[i]) {
				continue;
			} else {
				return false;
			}
		}
		return false;
	}
	MIGRtpsKeyHash& operator = (const MIGRtpsKeyHash& right) {
		this->length = right.length;

		memcpy(this->value, right.value, this->length);

		return *this;
	}

};

struct InstanceHandle_t {
	/*e We use guid to represent an instance */
	struct MIGRtpsKeyHash keyHash;

	int32_t isValid;

	InstanceHandle_t() :isValid(0) {}

	InstanceHandle_t(const InstanceHandle_t& other) {
		this->isValid = other.isValid;
		this->keyHash = other.keyHash;
	}

	InstanceHandle_t(InstanceHandle_t&& other) noexcept {
		this->isValid = other.isValid;
		this->keyHash = other.keyHash;
		memset(other.keyHash.value, 0, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		other.keyHash.length = 0;
		other.isValid = 0;
	}

	InstanceHandle_t& operator = (InstanceHandle_t&& other) noexcept {
		if (this != &other) {
			this->isValid = other.isValid;
			this->keyHash = other.keyHash;
			memset(other.keyHash.value, 0, MIG_RTPS_KEY_HASH_MAX_LENGTH);
			other.keyHash.length = 0;
			other.isValid = 0;
		}
		return *this;
	}

	InstanceHandle_t& operator = (const InstanceHandle_t& left) {
		this->isValid = left.isValid;
		this->keyHash = left.keyHash;
		return *this;
	}

	bool operator == (const InstanceHandle_t& right) const {
		if (this->keyHash != right.keyHash) {
			return false;
		}

		if (this->isValid != right.isValid) {
			return false;
		}

		return true;
	}

	bool operator != (const InstanceHandle_t& right) const {
		if (this->keyHash != right.keyHash) {
			return true;
		}

		if (this->isValid != right.isValid) {
			return true;
		}

		return false;
	}
	bool operator < (const InstanceHandle_t& other) const {
		return other.keyHash > this->keyHash;
	}
};

const InstanceHandle_t HANDLE_NIL;

struct BuiltinTopicKey_t {
	BUILTIN_TOPIC_KEY_TYPE_NATIVE value[4];

	/* wangyi Cyclone DDS 适配 添加构造函数初始化  */
	BuiltinTopicKey_t()
	{
		value[0] = 0;
		value[1] = 0;
		value[2] = 0;
		value[3] = 0;
	}

	bool operator==(const BuiltinTopicKey_t& other) const {
		for (int32_t i = 0; i < 4; ++i) {
			if (this->value[i] != other.value[i]) {
				return false;
			}
		}
		return true;
	}

	// 重载 != 操作符 (直接复用 == 操作符的结果)
	bool operator!=(const BuiltinTopicKey_t& other) const {
		return !(*this == other);
	}

	// 重载小于运算符
	bool operator<(const BuiltinTopicKey_t& other) const {
		for (int32_t i = 0; i < 4; ++i) {
			if (this->value[i] < other.value[i]) {
				return true;
			} else if (this->value[i] > other.value[i]) {
				return false;
			}
		}
		return false;
	}

};

/* 获取当前时间并赋值到Time_t结构数据中 */
static void CurrentTimeSinceUnixEpoch(
	int32_t& secs,
	uint32_t& nanosecs)
{
	using namespace std::chrono;
	auto t_since_epoch = system_clock::now().time_since_epoch();
	auto secs_t = duration_cast<seconds>(t_since_epoch);
	t_since_epoch -= secs_t;
	secs = static_cast<long>(secs_t.count());
	nanosecs = static_cast<unsigned long>(duration_cast<nanoseconds>(t_since_epoch).count());
}

struct Duration_t {
	int32_t sec;
	uint32_t nanosec;
	bool operator < (const Duration_t& other) const {
		if (this->sec < other.sec) {
			return true;
		} else if (this->sec ==other.sec) {
			if (this->nanosec < other.nanosec) {
				return true;
			}
		}
		return false;
	}
	bool operator > (const Duration_t& other) const {
		if (this->sec > other.sec) {
			return true;
		} else if (this->sec == other.sec) {
			if (this->nanosec > other.nanosec) {
				return true;
			}
		}
		return false;
	}
	bool operator <= (const Duration_t& other) const {
		if (this->sec < other.sec) {
			return true;
		} else if (this->sec == other.sec) {
			if (this->nanosec <= other.nanosec) {
				return true;
			}
		}
		return false;
	}
	bool operator >= (const Duration_t& other) const {
		if (this->sec > other.sec) {
			return true;
		} else if (this->sec == other.sec) {
			if (this->nanosec >= other.nanosec) {
				return true;
			}
		}
		return false;
	}
	bool operator == (const Duration_t& other) const {
		return (this->sec == other.sec) && (this->nanosec == other.nanosec);
	}
	bool operator != (const Duration_t& other) const {
		return (this->sec != other.sec) || (this->nanosec != other.nanosec);
	}

	Duration_t operator - (const Duration_t& other) const {
		Duration_t result;

		// 计算秒差
		result.sec = this->sec - other.sec;

		// 处理纳秒差（需借位时）
		if (this->nanosec >= other.nanosec) {
			result.nanosec = this->nanosec - other.nanosec;
		}
		else {
			result.sec -= 1;  // 借位：从秒中减1
			result.nanosec = 1000000000 + this->nanosec - other.nanosec;
		}

		// 标准化结果（确保纳秒在有效范围内）
		if (result.nanosec >= 1000000000) {
			result.sec += static_cast<int32_t>(result.nanosec / 1000000000);
			result.nanosec %= 1000000000;
		}

		return result;
	}

	/* wangyi 时间转换（Duration_t 格式转换为纳秒） */
	int64_t ToNs() const {
		int64_t nano = sec * static_cast<int64_t>(1000000000ULL);
		nano += nanosec;
		return nano;
	}

	/* wangyi 获取当前时间 */
	static void now(Duration_t& ret) {
		CurrentTimeSinceUnixEpoch(ret.sec, ret.nanosec);
	}
};

struct Time_t {
	int32_t sec;
	uint32_t nanosec;
	/* wangyi 获取当前时间 */
	static void now(Time_t& ret) {
		CurrentTimeSinceUnixEpoch(ret.sec, ret.nanosec);
	}

	/* wangyi 时间转换（Time_t 格式转换为纳秒） */
	int64_t ToNs() const
	{
		int64_t nano = sec * static_cast<int64_t>(1000000000ULL);
		nano += nanosec;
		return nano;
	}

	// 重载不等于运算符
	bool operator!=(const Time_t& other) const {
		return !(*this == other);
	}

	// 重载相等运算符
	bool operator==(const Time_t& other) const {
		return sec == other.sec && nanosec == other.nanosec;
	}

	// 重载小于运算符
	bool operator<(const Time_t& other) const {
		if (sec < other.sec) {
			return true;
		}
		else if (sec == other.sec) {
			return nanosec < other.nanosec;
		}
		return false;
	}

	// 重载大于运算符
	bool operator>(const Time_t& other) const {
		return other < *this;
	}
};

#define TIME_ZERO {0, 0}
#define TIME_INVALID {-1, 0xffffffff}
#define TIME_INFINITE {0x7fffffff, 0xffffffff}

// ----------------------------------------------------------------------
// Pre-defined values
// ----------------------------------------------------------------------

const	uint32_t			    LENGTH_UNLIMITED = -1;

const	int32_t			    DURATION_INFINITE_SEC = 0x7fffffff;
const	uint32_t	DURATION_INFINITE_NSEC = 0x7fffffff;

const	int32_t			    DURATION_ZERO_SEC = 0;
const	uint32_t	DURATION_ZERO_NSEC = 0;

const	int32_t			    TIME_INVALID_SEC = -1;
const	uint32_t	TIME_INVALID_NSEC = 0xffffffff;


using InstanceHandleSeq = Sequence<InstanceHandle_t>;
using QosPolicyId_t = int32_t;
using octet = unsigned char;
using StringSeq = Sequence<std::string>;

struct Property_t {
	std::string name;
	std::string value;
};
using PropertySeq = Sequence<Property_t>;

struct SampleInfo {
	/*  （READ或NOT_READ）-代表是否已读取过相应的数据样本  */
	SampleStateKind 		sample_state;
	/* （NEW或NOT_NEW）-代表DataReader是否已经观察到相关实例最新一代的样本 */
	ViewStateKind 			view_state;
	/* ALIVE，NOT_ALIVE_DISPOSED或NOT_ALIVE_NO_WRITERS）-代表实例当前是否存活，或者该实例已被处理以及它被处理的原因。 */
	InstanceStateKind 		instance_state;
	/* 代表DataWriter提供的写入数据样本的时间 */
	Time_t					source_timestamp;
	/* 用于在本地标识相应的实例 */
	InstanceHandle_t 		instance_handle;
	/* 用于在本地标识修改实例的DataWriter */
	InstanceHandle_t 		publication_handle;
	/* 代表实例在收到样本时实例由DataWriter显式处理后变为存活状态的次数 */
	int32_t 					disposed_generation_count;
	/* 代表实例因为没有写入者而被处理后重新变为存活状态的次数 */
	int32_t 					no_writers_generation_count;
	/* 代表read或take方法返回的合集中同一个实例的样本数 */
	int32_t 					sample_rank;
	/* 代表接收的样本与收到与同一实例集合中最新样本之间的代际差异（实例被处理后再次变为存活状态的次数） */
	int32_t 					generation_rank;
	/* 代表接收的样本与同一实例最新样本（可能不在返回的合集中）之间的代际差异（实例被处理后再次变为存活状态的次数） */
	int32_t					absolute_generation_rank;
	/* 代表DataSample是否包含数据，或者仅用于传递实例instance_state的更改 */
	bool 					valid_data;

	//TODO 临时添加，数据接收的序列号
	int32_t reception_sequence_number;

	//TODO jdxcz 20250610 
	int32_t write_sequence_number;

	SampleInfo()
		: sample_state(NOT_READ_SAMPLE_STATE),
		  view_state(NEW_VIEW_STATE),
		  instance_state(ALIVE_INSTANCE_STATE),
		  source_timestamp(TIME_INVALID),
		  instance_handle(HANDLE_NIL),
		  publication_handle(HANDLE_NIL),
		  disposed_generation_count(0), no_writers_generation_count(0), sample_rank(0),
		  generation_rank(0), absolute_generation_rank(0), reception_sequence_number(0),
		  valid_data(false)
		  
	{

	}
};

using SampleInfoSeq = SequenceData<SampleInfo>;

TRAVODDS_NAMESPACE_END

#endif
