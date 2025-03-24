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

#pragma once


#include  <map>
#include <list>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "AGIDDS_DLL.h"
#include "Sequence/sequence.h"
#include "States.h"
using std::vector;
using std::map;
using std::string;


AGIDDS_NAMESPACE_BEGIN


/* 待实现的数据类型 */
#define DOMAINID_TYPE_NATIVE 	long
#define HANDLE_TYPE_NATIVE		long
#define HANDLE_NIL_NATIVE 		0
#define BUILTIN_TOPIC_KEY_TYPE_NATIVE 	long

#define TheParticipantFactory
//#define PARTICIPANT_QOS_DEFAULT
//#define TOPIC_QOS_DEFAULT
//#define PUBLISHER_QOS_DEFAULT
//#define SUBSCRIBER_QOS_DEFAULT
//#define DATAWRITER_QOS_DEFAULT
//#define DATAREADER_QOS_DEFAULT
#define DATAWRITER_QOS_USE_TOPIC_QOS
#define DATAREADER_QOS_USE_TOPIC_QOS

#define Sequence vector

using DomainId_t = DOMAINID_TYPE_NATIVE;

#define MIG_RTPS_KEY_HASH_MAX_LENGTH 16

struct MIGRtpsKeyHash {
	unsigned char value[MIG_RTPS_KEY_HASH_MAX_LENGTH];

	unsigned int length;

	MIGRtpsKeyHash() {
		memset(value, 0, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	}

	bool operator == (const MIGRtpsKeyHash & right) const {
		if (this->length != right.length) {
			return false;
		}

		for (unsigned int i = 0; i < this->length; i++) {
			if (this->value[i] != right.value[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator != (const MIGRtpsKeyHash & right) const {
		if (this->length != right.length) {
			return true;
		}
		for (unsigned int i = 0; i < this->length; i++) {
			if (this->value[i] != right.value[i]) {
				return true;
			}
		}
		return false;
	}
	bool operator > (const MIGRtpsKeyHash & right) const {
		if (this->length != right.length) {
			return false;
		}
		for (unsigned int i = 0; i < this->length; i++) {
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
	MIGRtpsKeyHash& operator = (const MIGRtpsKeyHash & right) {
		this->length = right.length;

		memcpy(this->value, right.value, this->length);

		return *this;
	}

};

struct InstanceHandle_t {
	/*e We use guid to represent an instance */
	struct MIGRtpsKeyHash keyHash;

	int isValid;

	InstanceHandle_t() :isValid(0) {}
	InstanceHandle_t& operator = (const InstanceHandle_t& left) {
		this->isValid = left.isValid;
		this->keyHash = left.keyHash;
		return *this;
	}

	bool operator == (const InstanceHandle_t & right) const {
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
	BUILTIN_TOPIC_KEY_TYPE_NATIVE value[3];

	// 重载 == 操作符 (比较两个键是否相等)
	bool operator==(const BuiltinTopicKey_t& other) const {
		return (value == other.value) &&
			(value == other.value) &&
			(value == other.value);
	}

	// 重载 != 操作符 (直接复用 == 操作符的结果)
	bool operator!=(const BuiltinTopicKey_t& other) const {
		return !(*this == other);
	}

	// 重载 < 操作符 (用于排序或作为map的key)
	bool operator<(const BuiltinTopicKey_t& other) const {
		if (value != other.value) {
			return value < other.value;
		}
		if (value != other.value) {
			return value < other.value;
		}
		return value < other.value;
	}

};

struct Duration_t {
	long sec;
	unsigned long nanosec;
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
};

struct Time_t {
	long sec;
	unsigned long nanosec;
};

#define TIME_ZERO {0, 0}
#define TIME_INVALID {-1, 0xffffffff}
#define TIME_INFINITE {0x7fffffff, 0xffffffff}

// ----------------------------------------------------------------------
// Pre-defined values
// ----------------------------------------------------------------------

const	long			LENGTH_UNLIMITED = -1;

const	long			DURATION_INFINITE_SEC = 0x7fffffff;
const	unsigned long	DURATION_INFINITE_NSEC = 0x7fffffff;

const	long			DURATION_ZERO_SEC = 0;
const	unsigned long	DURATION_ZERO_NSEC = 0;

const	long			TIME_INVALID_SEC = -1;
const	unsigned long	TIME_INVALID_NSEC = 0xffffffff;


using InstanceHandleSeq = Sequence<InstanceHandle_t>;
using QosPolicyId_t = long;
using octet = unsigned char;
using StringSeq = Sequence<string>;


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
	long 					disposed_generation_count;
	/* 代表实例因为没有写入者而被处理后重新变为存活状态的次数 */
	long 					no_writers_generation_count;
	/* 代表read或take方法返回的合集中同一个实例的样本数 */
	long 					sample_rank;
	/* 代表接收的样本与收到与同一实例集合中最新样本之间的代际差异（实例被处理后再次变为存活状态的次数） */
	long 					generation_rank;
	/* 代表接收的样本与同一实例最新样本（可能不在返回的合集中）之间的代际差异（实例被处理后再次变为存活状态的次数） */
	long					absolute_generation_rank;
	/* 代表DataSample是否包含数据，或者仅用于传递实例instance_state的更改 */
	bool 					valid_data;

	//TODO 临时添加，数据接收的序列号
	int reception_sequence_number;

	SampleInfo() 
	{
		valid_data = false;
	}
};

using SampleInfoSeq = SequenceData<SampleInfo>;



AGIDDS_NAMESPACE_END
