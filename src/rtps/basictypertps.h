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
#ifndef TRAVODDS_RTPS_BASICTYPERTPS_H
#define TRAVODDS_RTPS_BASICTYPERTPS_H

#include "type/basictype.h"

TRAVODDS_NAMESPACE_BEGIN

// rtps entity attribute

struct GuidPrefix_t {
	octet value[12];
#ifdef DDSSECURITY
	// 重载小于运算符
	bool operator<(const GuidPrefix_t& other) const {
		for (int32_t i = 0; i < 12; ++i) {
			if (this->value[i] < other.value[i]) {
				return true;
			}
			else if (this->value[i] > other.value[i]) {
				return false;
			}
		}
		return false;
	}

	bool operator==(const GuidPrefix_t& other) const {
		for (int32_t i = 0; i < 12; ++i) {
			if (this->value[i] < other.value[i]) {
				return false;
			}
			else if (this->value[i] > other.value[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator!=(const GuidPrefix_t& other) const {
		for (int32_t i = 0; i < 12; ++i) {
			if (this->value[i] < other.value[i]) {
				return true;
			}
			else if (this->value[i] > other.value[i]) {
				return true;
			}
		}
		return false;
	}
#endif // DDSSECURITY
};

/**
 * @brief GuidPrefix_t结构体比较。
 *
 * @param [in] left  GuidPrefix_t结构类型数据左值。
 * @param [in] right GuidPrefix_t结构类型数据右值。
 *
 * @return 左右值相同返回true，不同则返回false。
 */
inline bool CompareGuidPrefix(const GuidPrefix_t& left, const GuidPrefix_t& right)
{
	for (int i = 0; i < 12; i++) {
		if (left.value[i] != right.value[i])
			return false;
	}

	return true;
}

const GuidPrefix_t GUIDPREFIX_UNKNOWN = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
typedef octet OctetArray3[3];
//!@brief 结构EntityId_t，GUID_t的实体id部分。
struct EntityId_t
{
	OctetArray3 entityKey;
	octet entityKind;
	
	/**
	 * @brief 默认构造函数。
	 */
	EntityId_t() : entityKey{ 0 }, entityKind(0) { };
	
	/**
	 * @brief 构造函数
	 * 
	 * @param key 实体id键值。
	 * @param kind 实体id类型。
	 */
	EntityId_t(const OctetArray3& key, const octet& kind)
		: entityKind(kind), entityKey{ 0 } {
		memcpy(&this->entityKey, &key, sizeof(OctetArray3));
	};

	/**
	 * @brief 拷贝构造函数
	 *
	 * @param other EntityId_t。
	 */
	EntityId_t(const EntityId_t& other)
		: entityKind(other.entityKind), entityKey{ 0 } {
		memcpy(&this->entityKey, &other.entityKey, sizeof(OctetArray3));
	}

	/**
	 * @brief 重载运算符 =
	 *
	 * @param right EntityId_t。
	 */
	EntityId_t operator = (const EntityId_t& right) {
		memcpy(&this->entityKey, &right.entityKey, sizeof(OctetArray3));
		this->entityKind = right.entityKind;

		return *this;
	}

	/**
	 * @brief 重载运算符 <
	 *
	 * @param right EntityId_t。
	 */
	bool operator < (const EntityId_t& right) const {
		for (int32_t i = 0; i < 3; ++i) {
			if (this->entityKey[i] < right.entityKey[i]) {
				return true;
			} else if (this->entityKey[i] > right.entityKey[i]) {
				return false;
			}
		}
		if (this->entityKind < right.entityKind) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 >
	 *
	 * @param right EntityId_t。
	 */
	bool operator > (const EntityId_t& right) const {
		for (int32_t i = 0; i < 3; ++i) {
			if (this->entityKey[i] > right.entityKey[i]) {
				return true;
			} else if (this->entityKey[i] < right.entityKey[i]) {
				return false;
			}
		}
		if (this->entityKind > right.entityKind) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 >=
	 *
	 * @param right EntityId_t。
	 */
	bool operator >= (const EntityId_t& right) const {
		for (int32_t i = 0; i < 3; ++i) {
			if (this->entityKey[i] > right.entityKey[i]) {
				return true;
			} else if (this->entityKey[i] < right.entityKey[i]) {
				return false;
			}
		}
		if (this->entityKind >= right.entityKind) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 <=
	 *
	 * @param right EntityId_t。
	 */
	bool operator <= (const EntityId_t& right) const {
		for (int32_t i = 0; i < 3; ++i) {
			if (this->entityKey[i] < right.entityKey[i]) {
				return true;
			} else if (this->entityKey[i] > right.entityKey[i]) {
				return false;
			}
		}
		if (this->entityKind <= right.entityKind) {
			return true;
		}
		return false;
	}

	/**
	 *@brief 重载运算符 ==
	 *
	 *@param right EntityId_t。
	 */
	//bool operator == (const EntityId_t& right) 
	//{
	//	int32_t N = 0;
	//	while (N < 3)
	//	{
	//		if (this->entityKey[N] != Right.entityKey[N])
	//			return false;
	//		++N;
	//	}
	//	if (this->entityKind != Right.entityKind)
	//		return false;

	//	return true;
	//}

	/**
	 * @brief 重载运算符 ==
	 *
	 * @param right EntityId_t。
	 */
	bool operator == (const EntityId_t& right) const {
		int32_t n = 0;
		while (n < 3) {
			if (this->entityKey[n] != right.entityKey[n]) {
				return false;
			}
			++n;
		}
		if (this->entityKind != right.entityKind) {
			return false;
		}

		return true;
	}

	/**
	 * @brief 重载运算符 !=
	 *
	 * @param right EntityId_t。
	 */
	bool operator != (const EntityId_t& right) const {
		int32_t n = 0;
		while (n < 3) {
			if (this->entityKey[n] != right.entityKey[n]) {
				return true;
			}
			++n;
		}
		if (this->entityKind != right.entityKind) {
			return true;
		}
		return false;
	}

};

const EntityId_t ENTITYID_UNKNOWN = { {0x00, 0x00, 0x00}, 0x00 };
//域参与者entityId
const EntityId_t ENTITYID_PARTICIPANT = { {0x00, 0x00, 0x01}, 0xc1 };
//内置主题发布端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_WRITER = { {0x00, 0x00, 0x02}, 0xc2 };
//内置主题订阅端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_READER = { {0x00, 0x00, 0x02}, 0xc7 };
//内置发布信息发布端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER = { {0x00, 0x00, 0x03}, 0xc2 };
//内置发布定西订阅端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER = { {0x00, 0x00, 0x03}, 0xc7 };
//内置订阅信息发布端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER = { {0x00, 0x00, 0x04}, 0xc2 };
//内置订阅信息订阅端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER = { {0x00, 0x00, 0x04}, 0xc7 };
//内置域参与者发布端entityId
const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER = { {0x00, 0x01, 0x00}, 0xc2 };
//内置域参与者订阅端entityId
const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER = { {0x00, 0x01, 0x00}, 0xc7 };
//内置域参与者信息发布端entityId
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER = { {0x00, 0x02, 0x00}, 0xc2 };
//内置域参与者信息订阅端entityId
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER = { {0x00, 0x02, 0x00}, 0xc7 };

#ifdef DDSSECURITY
//内置发布信息(安全)发布端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER = { { 0xff, 0x00, 0x03 }, 0xc2 };
//内置发布信息(安全)订阅端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER = { { 0xff, 0x00, 0x03 }, 0xc7 };
//内置订阅信息发布端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER = { { 0xff, 0x00, 0x04 }, 0xc2 };
//内置订阅信息订阅端entityId
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER = { { 0xff, 0x00, 0x04 }, 0xc7 };
//内置域参与者信息发布端entityId
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER = { { 0xff, 0x02, 0x00 }, 0xc2 };
//内置域参与者信息订阅端entityId
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER = { { 0xff, 0x02, 0x00 }, 0xc7 };
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER = { { 0x00, 0x02, 0x01 }, 0xc3 };
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER = { { 0x00, 0x02, 0x01 }, 0xc4 };
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER = { { 0xff, 0x02, 0x02 }, 0xc3 };
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER = { { 0xff, 0x02, 0x02 }, 0xc4 };
#endif // DDSSECURITY

//OMG规范定义的entityKind
//未知实体
#define ENTITYKIND_USER_UNKNOWN             0x00
//内置未知端点
#define ENTITYKIND_BUILTIN_UNKNOWN          0xc0
//内置域参与者端点
#define ENTITYKIND_BUILTIN_PARTICIPANT      0xc1
//用户带key发布端
#define ENTITYKIND_USER_WRITER_WITH_KEY     0x02
//内置带key发布端
#define ENTITYKIND_BUILTIN_WRITER_WITH_KEY  0xc2
//用户发布端
#define ENTITYKIND_USER_WRITER_NO_KEY       0x03
//内置发布端
#define ENTITYKIND_BUILTIN_WRITER_NO_KEY    0xc3
//用户带key订阅端
#define ENTITYKIND_USER_READER_WITH_KEY     0x07
//内置带key订阅端
#define ENTITYKIND_BUILTIN_READER_WITH_KEY  0xc7
//用户订阅端
#define ENTITYKIND_USER_READER_NO_KEY       0x04
//内置订阅端
#define ENTITYKIND_BUILTIN_READER_NO_KEY    0xc4

//！@brief GUID_t结构体，实体标识符，在DDS RTPS域中唯一。
struct GUID_t {
	GuidPrefix_t prefix;
	EntityId_t entityId;

	/**
	 * @brief 默认构造函数。构造未知的GUID。
	 */
	GUID_t() {
		memset(this->prefix.value, 0, sizeof(GuidPrefix_t));
		this->entityId = ENTITYID_UNKNOWN;
	}
	/**
	 * @brief 拷贝构造函数。
	 * @param guid GUID_t。
	 */
	GUID_t(const GUID_t& guid) {
		memcpy(&this->prefix, &guid.prefix, sizeof(GuidPrefix_t));
		this->entityId = guid.entityId;
	}
	/**
	 * @brief 构造函数。
	 * @param prefix guid前缀
	 * @param id 实体id
	 */
	GUID_t(const GuidPrefix_t& prefix, const EntityId_t& id) {
		memcpy(&this->prefix, &prefix, sizeof(GuidPrefix_t));
		this->entityId = id;
	}

	/**
	 * @brief 重载运算符 ==
	 *
	 * @param right GUID_t。
	 */
	bool operator ==(const GUID_t& right) const {
		int32_t m = 0;
		while (m < 12) {
			if (this->prefix.value[m] != right.prefix.value[m]) {
				return false;
			}
			++m;
		}
		if (this->entityId != right.entityId) {
			return false;
		}

		return true;
	}

	/**
	 * @brief 重载运算符 !=
	 *
	 * @param right GUID_t。
	 */
	bool operator != (const GUID_t& right) const {
		int32_t m = 0;
		while (m < 12) {
			if (this->prefix.value[m] != right.prefix.value[m]) {
				return true;
			}
			++m;
		}
		if (this->entityId != right.entityId) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 <
	 *
	 * @param right GUID_t。
	 */
	bool operator < (const GUID_t& right) const {
		int32_t m = 0;
		while (m < 12) {
			if (this->prefix.value[m] > right.prefix.value[m]) {
				return false;
			} else if (this->prefix.value[m] < right.prefix.value[m]) {
				return true;
			}
			++m;
		}
		if (this->entityId < right.entityId) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 >
	 *
	 * @param right GUID_t。
	 */
	bool operator > (const GUID_t& right) const {
		int32_t m = 0;
		while (m < 12) {
			if (this->prefix.value[m] < right.prefix.value[m]) {
				return false;
			} else if (this->prefix.value[m] > right.prefix.value[m]) {
				return true;
			}
			++m;
		}
		if (this->entityId > right.entityId) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 =
	 *
	 * @param right GUID_t。
	 */
	GUID_t operator = (const GUID_t& right) {
		memcpy(this->prefix.value, right.prefix.value, sizeof(GuidPrefix_t));
		this->entityId = right.entityId;

		return *this;
	}
	/**
	 * @brief 重载运算符 =
	 *
	 * @param right GuidPrefix_t。
	 */
	GUID_t operator = (const GuidPrefix_t& right) {
		memcpy(this->prefix.value, right.value, sizeof(GuidPrefix_t));
		return *this;
	}


#ifdef DDSSECURITY
	GUID_t& operator=(const BuiltinTopicKey_t& key) {
		memcpy(this, &key, sizeof(GUID_t));
		return *this;
	}

	GUID_t& operator=(const BuiltinTopicKey_t* key) {
		if (key != nullptr) {
			memcpy(this, key, sizeof(GUID_t));
		}
		return *this;
	}

    /**
     * @brief 判断是否是安全内置实体
     */
    bool IsSecureBuiltinEntity() {
        if (entityId.entityKey[0] == (unsigned char)0xff)
        {
            return true;
        }
        if (entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER)
        {
            return true;
        }
        return false;
    }

    /**
     * @brief 判断是否是用户自定义实体
     */
    bool IsUserDefinedEntity() {
        return (((entityId).entityKind & 0xc0) == 0x00);
    }
	
#endif // DDSSECURITY
};

const GUID_t GUID_UNKNOWN = { GUIDPREFIX_UNKNOWN,ENTITYID_UNKNOWN };

namespace Conversion {
	/**
	 * @brief BuiltinTopicKey_t 结构转换为GUID_t结构的转换函数
	 *
	 * @param key BuiltinTopicKey_t结构数据。
	 * 
	 * @return GUID_t结构数据。
	 */
	inline GUID_t ToGuid(const BuiltinTopicKey_t& key) {
		GUID_t guid;
		memcpy(&guid, &key, sizeof(GUID_t));
		return guid;
	}
	/**
	 * @brief InstanceHandle_t 结构转换为GUID_t结构的转换函数
	 *
	 * @param handle InstanceHandle_t结构数据。
	 *
	 * @return GUID_t结构数据。
	 */
	inline GUID_t ToGuid(const InstanceHandle_t& handle) {
		GUID_t guid;
		memcpy(&guid, handle.keyHash.value, sizeof(GUID_t));
		return guid;
	}
	/**
	 * @brief GUID_t 结构转换为InstanceHandle_t结构的转换函数
	 *
	 * @param guid GUID_t结构数据。
	 *
	 * @return InstanceHandle_t结构数据。
	 */
	inline InstanceHandle_t ToInstanceHandle(const GUID_t& guid) {
		InstanceHandle_t handle;
		memcpy(handle.keyHash.value, &guid, sizeof(GUID_t));
		handle.isValid = true;
		return handle;
	}

	/**
	 * @brief BuiltinTopicKey_t 结构转换为InstanceHandle_t结构的转换函数
	 *
	 * @param key BuiltinTopicKey_t结构数据。
	 *
	 * @return InstanceHandle_t结构数据。
	 */
	inline InstanceHandle_t ToInstanceHandle(const BuiltinTopicKey_t& key) {
		InstanceHandle_t handle;
		memcpy(handle.keyHash.value, &key, sizeof(GUID_t));
		handle.isValid = true;
		return handle;
	}
	/**
	 * @brief GUID_t 结构转换为BuiltinTopicKey_t结构的转换函数
	 *
	 * @param guid GUID_t结构数据。
	 *
	 * @return BuiltinTopicKey_t结构数据。
	 */
	inline BuiltinTopicKey_t ToBuiltinTopicKey(const GUID_t& guid) {
		BuiltinTopicKey_t key;
		memcpy(&key, &guid, sizeof(GUID_t));
		return key;
	}
	/**
	 * @brief InstanceHandle_t 结构转换为BuiltinTopicKey_t结构的转换函数
	 *
	 * @param handle InstanceHandle_t结构数据。
	 *
	 * @return BuiltinTopicKey_t结构数据。
	 */
	inline BuiltinTopicKey_t ToBuiltinTopicKey(const InstanceHandle_t& handle) {
		BuiltinTopicKey_t key;
		memcpy(&key, handle.keyHash.value, sizeof(GUID_t));
		return key;
	}
}

using VendorId_t = std::array<octet, 2>;//octet[2];

const VendorId_t VENDORID_UNKNOWN = { 0,0 };
const VendorId_t VENDORID_PROSIMA = { 0,0 };

struct SequenceNumber_t {
	int32_t high = 0;
	uint32_t low = 0;

	/**
	 * @brief 重载运算符 <
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator < (const SequenceNumber_t& right) const {

		if (high == right.high && low < right.low) {
			return true;
		}
		else if (high < right.high) {
			return true;
		}
		
		return false;
	}

	/**
	 * @brief 重载运算符 <=
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator <= (const SequenceNumber_t& right) const {
		
		if (high == right.high && low <= right.low) {
			return true;
		}
		else if (high < right.high) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 ==
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator == (const SequenceNumber_t& right) const {
		if (low == right.low && high == right.high) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 >=
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator >= (const SequenceNumber_t& right) const {
		if (high == right.high && low >= right.low) {
			return true;
		}
		else if(high > right.high){
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 >
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator > (const SequenceNumber_t& right) const {
		if (high == right.high && low > right.low) {
			return true;
		}
		else if (high > right.high) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 !=
	 *
	 * @param right SequenceNumber_t。
	 */
	bool operator != (const SequenceNumber_t& right) const {
		if ((this->high == right.high) && (this->low == right.low)) {
			return false;
		} else {
			return true;
		}
	}

	/**
	 * @brief 重载运算符 ++
	 *
	 * @param int32_t。
	 */
	SequenceNumber_t operator ++ (int32_t) {
		this->low++;
		if (0 == this->low) {
			this->high++;
		}
		return *this;
	}

	/**
	 * @brief 重载运算符 +
	 *
	 * @param Add int32_t 加数。
	 */
	SequenceNumber_t operator + (const int32_t Add) const {
		SequenceNumber_t seq = *this;
		if (seq.low > (uint32_t)-1 - Add) {
			++seq.high;
		}
		seq.low += Add;

		return seq;
	}

	/**
	 * @brief 重载运算符 -
	 *
	 * @param sub int32_t 减数。
	 */
 	SequenceNumber_t operator - (const int32_t sub) const {
 		SequenceNumber_t seq = *this;
 		if (this->low < sub) {
 			seq.high--;
 			seq.low = this->low + (0xffffffff - sub);
 		}
 		else {
 			seq.low = seq.low - sub;
 		}
 		return seq;
 	}
 	
	/**
	 * @brief 重载运算符 -
	 *
	 * @param sub SequenceNumber_t 减数。
	 */
 	int32_t operator - (const SequenceNumber_t& sub) const {
 		SequenceNumber_t seq = *this;
 		if (this->low < sub.low) {
 			seq.high--;
 			seq.low = this->low + (0xffffffff - sub.low);
		}
		else {
			seq.low = this->low - sub.low;
		}
 		seq.high = seq.high - sub.high;
 
 		return seq.low;
 	}
};
// using ChangeCount_t = SequenceNumber_t;

// 未知序列号
const SequenceNumber_t SEQUENCENUMBER_UNKNOWN = { -1, 0 };
const SequenceNumber_t SEQUENCENUMBER_INVALID = { -2, 0 };
const SequenceNumber_t SEQUENCENUMBER_ZERO = { 0, 0 };
const SequenceNumber_t SEQUENCENUMBER_FIRST = { 0, 1 };
// #define SEQUENCENUMBER_UNKNOWN {-1,0}
// #define SEQUENCENUMBER_INVALID {-2,0}

using SeqNumberVec = std::vector<SequenceNumber_t>;

struct ChangeCount_t {
	int32_t high;
	uint32_t low;
};

using FragmentNumber_t = uint32_t;


#define LOCATOR_ADDRESS_INVALID {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

enum LocatorKind : int32_t
{
	LOCATOR_KIND_INVALID = -1,
	LOCATOR_KIND_RESERVED = 0,
	LOCATOR_KIND_UDPv4 = 1,
	LOCATOR_KIND_UDPv6 = 2,
	LOCATOR_KIND_TCPv4 = 3,
	LOCATOR_KIND_TCPv6 = 4,
	LOCATOR_KIND_SHM = 5
};
const uint32_t LOCATOR_PORT_INVALID = 0;

struct Locator_t {

	int32_t kind;
	/// Network port
	uint32_t port;
	/// IP address
	octet address[16];

	/**
	 * @brief 构造函数。
	 */
	Locator_t() :kind(LOCATOR_KIND_INVALID), port(0) {
		memset(address, 0, 16);
	}
	/**
	 * @brief 析构函数。
	 */
	virtual ~ Locator_t() {
		memset(address, 0, 16);
	}
	/**
	 * @brief 重载运算符 -
	 *
	 * @param locator Locator_t。
	 */
	bool operator < (const Locator_t& locator) const {
		for (int32_t i = 0; i < 16; i++) {
			if (this->address[i] < locator.address[i]) {
				return true;
			}

			if (this->address[i] > locator.address[i]) {
				return false;
			}
		}

		if (this->port < locator.port) {
			return true;
		}

		if (this->kind < locator.kind) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 -
	 *
	 * @param locator Locator_t。
	 */
	bool operator > (const Locator_t& locator) const {
		for (int32_t i = 0; i < 16; i++) {
			if (this->address[i] > locator.address[i]) {
				return true;
			}

			if (this->address[i] < locator.address[i]) {
				return false;
			}
		}

		if (this->port > locator.port) {
			return true;
		}

		if (this->kind > locator.kind) {
			return true;
		}

		return false;
	}

	/**
	 * @brief 重载运算符 ==
	 *
	 * @param locator Locator_t。
	 */
	bool operator == (const Locator_t& locator) const {

		for (int32_t i = 0; i < 16; i++) {
			if (this->address[i] != locator.address[i]) {
				return false;
			}
		}

		if (this->port != locator.port) {
			return false;
		}
		if (this->kind != locator.kind) {
			return false;
		}
		return true;
	}
};

using LocatorVec = std::vector<Locator_t>;
using LocatorSeq = Sequence<Locator_t>;

#define LOCATOR_INVALID {LOCATOR_KIND_INVALID, LOCATOR_PORT_INVALID, LOCATOR_ADDRESS_INVALID}


using ReliabilityKind_t = int32_t;
const int32_t BEST_EFFORT = 1;
const int32_t RELIABLE = 2;

using Count_t = int32_t;

struct ProtocolVersion_t {
	octet major;
	octet minor;
};

#define PROTOCOLVERSION_1_0 {1,0}
#define PROTOCOLVERSION_1_1 {1,1}
#define PROTOCOLVERSION_2_0 {2,0}
#define PROTOCOLVERSION_2_1 {2,1}
#define PROTOCOLVERSION_2_2 {2,2}
#define PROTOCOLVERSION_2_5 {2,5}
#define PROTOCOLVERSION PROTOCOLVERSION_2_5

/* wangyi add */
#define BIT(i) (1U << static_cast<unsigned>(i))

using KeyHash_t = octet[16];
using StatusInfo_t = octet[4];

struct ContentFilterProperty_t {
	std::string contentFilteredTopicName;
	std::string relatedTopicName;
	std::string filterClassName;
	std::string filterExpression;
	Sequence<std::string> expressionParameters;
};

using FilterResult_t = Sequence<int32_t>;
using FilterSignature_t = std::array<int32_t, 4>;
using FilterSignatureSequence = Sequence<FilterSignature_t>;
struct ContentFilterInfo_t {
	FilterResult_t filterResult;
	FilterSignatureSequence filterSignatures;
};

using EntityName_t = std::string;

using ParameterId_t = uint16_t;
struct Parameter_t
{
	ParameterId_t parameterId;
	uint16_t length;
	Sequence<octet> value; // Pseudo-IDL: array of non-const length
};
using ParameterList_t = std::vector<Parameter_t>;

struct OriginalWriterInfo_t {
	GUID_t originalWriterGUID;
	SequenceNumber_t originalWriterSN;
	ParameterList_t originalWriterQos;
};

using GroupDigest_t = octet[4];

struct WriterGroupInfo_t
{
	GroupDigest_t writerSet;
};

using BuiltinEndpointSet_t = uint32_t;
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER        0x00000001 << 0
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR         0x00000001 << 1
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER        0x00000001 << 2
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR         0x00000001 << 3
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER       0x00000001 << 4
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR        0x00000001 << 5
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_PROXY_ANNOUNCER  0x00000001 << 6
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_PROXY_DETECTOR   0x00000001 << 7
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_STATE_ANNOUNCER  0x00000001 << 8
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_STATE_DETECTOR   0x00000001 << 9
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER   0x00000001 << 10
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER   0x00000001 << 11

#ifdef DDSSECURITY
/* 安全相关扩展 */
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER        0x00000001 << 12
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR         0x00000001 << 13
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER       0x00000001 << 14
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR        0x00000001 << 15
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_WRITER   0x00000001 << 16
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_READER   0x00000001 << 17
#define BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_WRITER   0x00000001 << 18
#define BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_READER   0x00000001 << 19
#define BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_WRITER   0x00000001 << 20
#define BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_READER   0x00000001 << 21
#endif // DDSSECURITY

#define DISC_BUILTIN_ENDPOINT_TOPICS_ANNOUNCER             0x00000001 << 28
#define DISC_BUILTIN_ENDPOINT_TOPICS_DETECTOR              0x00000001 << 29

using BuiltinEndpointQos_t = uint32_t;
#define	BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER        0x1 << 0;
#define RELIABLE_PARTICIPANT_MESSAGE_READE                 0x1 << 1;
#define BEST_EFFORT_PARTICIPANT_MESSAGE_WRITER             0x1 << 2;
#define RELIABLE_PARTICIPANT_MESSAGE_WRITER                0x1 << 3;

using ProtocolId_t = octet[4];
const ProtocolId_t PROTOCOL_RTPS = { 'R','T','P','S' };

// RTPS HeaderExtension
using MessageLength_t = uint32_t;
const MessageLength_t MESSAGE_LENGTH_INVALID = 0;
using UExtension4_t = octet[4];
using WExtension8_t = octet[8];
using Checksum32_t = octet[4];
using Checksum64_t = octet[8];
using Checksum128_t = octet[16];

struct TopicKind_t {
	int32_t value;
};
#define NO_KEY 1
#define WITH_KEY 2

enum ChangeKind_t {
	ALIVE,
	ALIVE_FILTERED,
	NOT_ALIVE_DISPOSED,
	NOT_ALIVE_UNREGISTERED,
	NOT_ALIVE_DISPOSED_UNREGISTERED	
};


const uint32_t DATAMESSAGE_LIMIT = 62 * 1024;
const uint32_t BUFF_SHIFT = 1024;
const uint16_t OCTETS_TO_INLINEQOS_LEN = 16;

//PSM
struct LocatorUDPv4_t
{
	uint32_t address;
	uint32_t port;
};
#define LOCATORUDPv4_INVALID {0, 0}


#define PORT_BASE_NUMBER 7400
#define DOMAINID_GAIN 250
#define PARTICIPANTID_GAIN	2
#define ADDITIONAL_OFFSETS_D0 0
#define ADDITIONAL_OFFSETS_D1 10
#define ADDITIONAL_OFFSETS_D2 1
#define ADDITIONAL_OFFSETS_D3 11

//多播发现端口
#define SPDP_WELL_KNOWN_MULTICAST_PORT(ulDomainId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (uint32_t)ulDomainId + ADDITIONAL_OFFSETS_D0
//单播发现端口
#define SPDP_WELL_KNOWN_UNICAST_PORT(ulDomainId, ulParticipantId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (uint32_t)ulDomainId + ADDITIONAL_OFFSETS_D1 + \
    PARTICIPANTID_GAIN * (uint32_t)ulParticipantId
//用户多播端口
#define USER_DEFAULT_MULTICAST_PORT(ulDomainId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (uint32_t)ulDomainId + ADDITIONAL_OFFSETS_D2
//用户单播端口
#define USER_DEFAULT_UNICAST_PORT(ulDomainId, ulParticipantId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (uint32_t)ulDomainId + ADDITIONAL_OFFSETS_D3 + \
    PARTICIPANTID_GAIN * (uint32_t)ulParticipantId


TRAVODDS_NAMESPACE_END

#endif // ! TRAVODDS_RTPS_BASICTYPERTPS_H
