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
#ifndef TRAVODDS_TYPE_GUID_H
#define TRAVODDS_TYPE_GUID_H

#include "type/basictype.h"

TRAVODDS_NAMESPACE_BEGIN

typedef octet OctetArray12[12];
struct GuidPrefix_t {
	OctetArray12 value;

	GuidPrefix_t() : value{0}{}

	GuidPrefix_t(const GuidPrefix_t& other) {
		memcpy(&this->value, &other.value, sizeof(OctetArray12));
	}

	GuidPrefix_t(const OctetArray12& value) {
		memcpy(&this->value, &value, sizeof(OctetArray12));
	}

	GuidPrefix_t& operator = (const GuidPrefix_t& right) {
		memcpy(&this->value, &right.value, sizeof(OctetArray12));
		return *this;
	}

	bool operator<(const GuidPrefix_t& other) const {
		return (memcmp(this->value, other.value, sizeof(OctetArray12)) < 0);
	}

	bool operator==(const GuidPrefix_t& other) const {
		return (memcmp(this->value, other.value, sizeof(OctetArray12)) == 0);
	}

	bool operator!=(const GuidPrefix_t& other) const {
		return (memcmp(this->value, other.value, sizeof(OctetArray12)) != 0);
	}
};

const GuidPrefix_t GUIDPREFIX_UNKNOWN = {{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }};

typedef octet OctetArray3[3];
//!@brief 结构EntityId_t，GUID_t的实体id部分。
struct EntityId_t
{
	OctetArray3 entityKey;
	octet entityKind;

	/**
	 * @brief 默认构造函数。
	 */
	EntityId_t() : entityKey{ 0 }, entityKind(0) {};

	/**
	 * @brief 构造函数
	 *
	 * @param key 实体id键值。
	 * @param kind 实体id类型。
	 */
	EntityId_t(const OctetArray3& key, const octet& kind)
		: entityKind(kind) {
		memcpy(&this->entityKey, &key, sizeof(OctetArray3));
	};

	/**
	 * @brief 拷贝构造函数
	 *
	 * @param other EntityId_t。
	 */
	EntityId_t(const EntityId_t& other)
		: entityKind(other.entityKind) {
		memcpy(&this->entityKey, &other.entityKey, sizeof(OctetArray3));
	}

	/**
	 * @brief 重载运算符 =
	 *
	 * @param right EntityId_t。
	 */
	EntityId_t& operator = (const EntityId_t& right) {
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
	    int32_t ret = memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3));
		if (ret < 0 ||
			(ret == 0 && this->entityKind < right.entityKind)) {
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
		int32_t ret = memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3));
		if (ret > 0 ||
			(ret == 0 && this->entityKind > right.entityKind)) {
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
		return memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3)) == 0 && 
		    this->entityKind >= right.entityKind;
	}

	/**
	 * @brief 重载运算符 <=
	 *
	 * @param right EntityId_t。
	 */
	bool operator <= (const EntityId_t& right) const {
		return memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3)) == 0 &&
			this->entityKind <= right.entityKind;
	}

	 /**
	  * @brief 重载运算符 ==
	  *
	  * @param right EntityId_t。
	  */
	bool operator == (const EntityId_t& right) const {
		return memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3)) == 0 &&
			this->entityKind == right.entityKind;
	}

	/**
	 * @brief 重载运算符 !=
	 *
	 * @param right EntityId_t。
	 */
	bool operator != (const EntityId_t& right) const {
		return memcmp(this->entityKey, right.entityKey, sizeof(OctetArray3)) != 0 ||
			this->entityKind != right.entityKind;
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
	GUID_t() {}
	/**
	 * @brief 拷贝构造函数。
	 * @param guid GUID_t。
	 */
	GUID_t(const GUID_t& guid) : 
	    prefix(guid.prefix),
		entityId(guid.entityId){
	}
	/**
	 * @brief 构造函数。
	 * @param prefix guid前缀
	 * @param id 实体id
	 */
	GUID_t(const GuidPrefix_t& prefix, const EntityId_t& id) :
		prefix(prefix),
		entityId(id) {
	}

	/**
	 * @brief 重载运算符 ==
	 *
	 * @param right GUID_t。
	 */
	bool operator ==(const GUID_t& right) const {
	   
		return this->prefix == right.prefix && this->entityId == right.entityId;
	}

	/**
	 * @brief 重载运算符 !=
	 *
	 * @param right GUID_t。
	 */
	bool operator != (const GUID_t& right) const {
		return this->prefix != right.prefix || this->entityId != right.entityId;
	}

	/**
	 * @brief 重载运算符 <
	 *
	 * @param right GUID_t。
	 */
	bool operator < (const GUID_t& right) const {
		int32_t ret = memcmp(this->prefix.value, right.prefix.value, sizeof(OctetArray12));
		if (ret < 0 ||
			(ret == 0 && this->entityId < right.entityId)) {
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
		int32_t ret = memcmp(this->prefix.value, right.prefix.value, sizeof(OctetArray12));
		if (ret > 0 ||
			(ret == 0 && this->entityId > right.entityId)) {
			return true;
		}
		return false;
	}

	/**
	 * @brief 重载运算符 =
	 *
	 * @param right GUID_t。
	 */
	GUID_t& operator = (const GUID_t& right) {
	    this->prefix = right.prefix;
		this->entityId = right.entityId;
		return *this;
	}
	/**
	 * @brief 重载运算符 =
	 *
	 * @param right GuidPrefix_t。
	 */
	GUID_t& operator = (const GuidPrefix_t& right) {
		this->prefix = right;
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

TRAVODDS_NAMESPACE_END

#endif
