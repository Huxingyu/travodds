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
#include  "BasicType.h"

AGIDDS_NAMESPACE_BEGIN

struct GuidPrefix_t {
	octet value[12];
};

const GuidPrefix_t GUIDPREFIX_UNKNOWN = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
typedef octet OctetArray3[3];
struct EntityId_t
{
	OctetArray3 entityKey;
	octet entityKind;

	//重载运算符 =
	EntityId_t operator = (const EntityId_t &right) {
		memcpy(&this->entityKey, &right.entityKey, sizeof(OctetArray3));
		memcpy(&this->entityKind, &right.entityKind, sizeof(octet));

		return *this;
	}
	//重载运算符 <
	bool operator < (const EntityId_t &right) const {
		for (int i = 0; i < 3; ++i) {
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
	//重载运算符 >
	bool operator > (const EntityId_t &right) const {
		for (int i = 0; i < 3; ++i) {
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
	//重载运算符 >=
	bool operator >= (const EntityId_t &right) const {
		for (int i = 0; i < 3; ++i) {
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
	//重载运算符 <=
	bool operator <= (const EntityId_t &right) const {
		for (int i = 0; i < 3; ++i) {
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
	//重载运算符 ==
	//bool operator == (const EntityId_t &Right) 
	//{
	//	int N = 0;
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

	bool operator == (const EntityId_t &right) const {
		int n = 0;
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
	//重载运算符 !=
	bool operator != (const EntityId_t &right) const {
		int n = 0;
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

struct GUID_t {
	GuidPrefix_t prefix;
	EntityId_t entityId;

	GUID_t() {
		memset(this->prefix.value, 0, sizeof(GuidPrefix_t));
		this->entityId = ENTITYID_UNKNOWN;
	}
	GUID_t(GuidPrefix_t prefix, EntityId_t id) {
		memcpy(this->prefix.value, prefix.value, sizeof(GuidPrefix_t));
		this->entityId = id;
	}

	//重载运算符==
	bool operator ==(const GUID_t &right) const {
		int m = 0;
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

	//重载运算符 !=
	bool operator != (const GUID_t &right) const {
		int m = 0;
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

	//重载运算符 <
	bool operator < (const GUID_t &right) const {
		int m = 0;
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

	//重载运算符 >
	bool operator > (const GUID_t &right) const {
		int m = 0;
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

	//重载运算符 =
	GUID_t operator = (const GUID_t &right) {
		memcpy(this->prefix.value, right.prefix.value, sizeof(GuidPrefix_t));
		this->entityId = right.entityId;

		return *this;
	}
	GUID_t operator = (const GuidPrefix_t &right) {
		memcpy(this->prefix.value, right.value, sizeof(GuidPrefix_t));
		return *this;
	}
};

const GUID_t GUID_UNKNOWN = { GUIDPREFIX_UNKNOWN,ENTITYID_UNKNOWN };

struct SequenceNumber_t {
	int high;
	unsigned int low;

	bool operator < (const SequenceNumber_t& right) const {
		if (low < right.low) {
			return true;
		}
		return false;
	}

	bool operator <= (const SequenceNumber_t& right) const {
		if (low <= right.low) {
			return true;
		}
		return false;
	}

	bool operator == (const SequenceNumber_t& right) const {
		if (low == right.low) {
			return true;
		}
		return false;
	}

	bool operator >= (const SequenceNumber_t& right) const {
		if (low >= right.low) {
			return true;
		}
		return false;
	}

	bool operator > (const SequenceNumber_t& right) const {
		if (low > right.low) {
			return true;
		}
		return false;
	}

	bool operator != (const SequenceNumber_t &right) const {
		if ((this->high == right.high) && (this->low == right.low)) {
			return false;
		} else {
			return true;
		}
	}

	SequenceNumber_t operator ++ (int) {
		this->low++;
		if (0 == this->low) {
			this->high++;
		}
		return *this;
	}

	SequenceNumber_t operator + (const long Add) const {
		SequenceNumber_t seq = *this;
		if (seq.low > (unsigned long)-1 - Add) {
			++seq.high;
		}
		seq.low += Add;

		return seq;
	}
};

using SeqNumberVec = vector<SequenceNumber_t>;

#define SEQUENCENUMBER_UNKNOWN {-1,0}

#define SEQUENCENUMBER_INVALID {-2,0}

#define LOCATOR_ADDRESS_INVALID {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define LOCATOR_PORT_INVALID  0
#define LOCATOR_KIND_INVALID  -1
#define LOCATOR_KIND_RESERVED  0
#define LOCATOR_KIND_UDPv4   1
#define LOCATOR_KIND_UDPv6   2

struct Locator_t {

	int kind;
	/// Network port
	unsigned int port;
	/// IP address
	octet address[16];

	Locator_t() :kind(LOCATOR_KIND_INVALID), port(0) {
		memset(address, 0, 16);
	}

	bool operator < (const Locator_t& locator) const {
		for (int i = 0; i < 16; i++) {
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


	bool operator > (const Locator_t& locator) const {
		for (int i = 0; i < 16; i++) {
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

	bool operator == (Locator_t& locator) const {
		for (int i = 0; i < 16; i++) {
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

using LocatorVec = vector<Locator_t>;
using LocatorSeq = Sequence<Locator_t>;

#define LOCATOR_INVALID {LOCATOR_KIND_INVALID, LOCATOR_PORT_INVALID, LOCATOR_ADDRESS_INVALID}



struct TopicKind_t {
	int value;
};
#define NO_KEY 1
#define WITH_KEY 2

enum ChangeKind_t {
	ALIVE,
	NOT_ALIVE_DISPOSED,
	NOT_ALIVE_UNREGISTERED

};

struct ReliabilityKind_t {
	int value;
};
#define BEST_EFFORT 1
#define RELIABLE 3

struct ProtocolVersion_t {
	octet major;
	octet minor;
};

#define PROTOCOLVERSION_1_0 {1,0}
#define PROTOCOLVERSION_1_1 {1,1}
#define PROTOCOLVERSION_2_0 {2,0}
#define PROTOCOLVERSION_2_1 {2,1}
#define PROTOCOLVERSION_2_2 {2,2}
#define PROTOCOLVERSION PROTOCOLVERSION_2_2


#define PROTOCOLVERSION_1_0 {1,0}
#define PROTOCOLVERSION_1_1 {1,1}
#define PROTOCOLVERSION_2_0 {2,0}
#define PROTOCOLVERSION_2_1 {2,1}
#define PROTOCOLVERSION_2_2 {2,2}
#define PROTOCOLVERSION PROTOCOLVERSION_2_2


using OctetArray2 = octet[2];
struct VendorId_t {
	OctetArray2 vendorId;
};

#define VENDORID_UNKNOWN {0,0}

// rtps message

struct FragmentNumber_t {
	unsigned int value;
};

struct Count_t {
	int value;
};

using OctetArray16 = octet[16];
struct KeyHash_t {
	OctetArray16 value;
};

using OctetArray4 = octet[4];
struct StatusInfo_t {
	OctetArray4 value;
};

struct ParameterId_t {
	short value;
};

using String256 = string ;
using StringSequence = Sequence<string>;
struct ContentFilterProperty_t {
	String256 contentFilteredTopicName;
	String256 relatedTopicName;
	String256 filterClassName;
	string filterExpression;
	StringSequence expressionParameters;
};

using FilterResult_t = Sequence<int>;
//TODO 需要重新定义
// using FilterSignature_t = int[4];
// using FilterSignatureSequence =Sequence<FilterSignature_t>;
//
// struct ContentFilterInfo_t {
// 	FilterResult_t filterResult;
// 	FilterSignatureSequence filterSignatures;
// };

struct Property_t {
	string name;
	string value;
};

struct EntityName_t {
	string name;
};


struct OriginalWriterInfo_t {
	GUID_t originalWriterGUID;
	SequenceNumber_t originalWriterSN;
	//ParameterList originalWriterQos;
};

using BuiltinEndpointSet_t =  unsigned int ;
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

const unsigned int DATAMESSAGE_LIMIT = 63 * 1024;
const unsigned int BUFF_SHIFT = 1024;

//PSM
struct LocatorUDPv4_t
{
	unsigned long address;
	unsigned long port;
};
#define LOCATORUDPv4_INVALID {0, 0}


#define PORT_BASE_NUMBER 7400
#define DOMAINID_GAIN 250
#define PARTICIPANTID_GAIN	3
#define ADDITIONAL_OFFSETS_D0 0
#define ADDITIONAL_OFFSETS_D1 10
#define ADDITIONAL_OFFSETS_D2 1
#define ADDITIONAL_OFFSETS_D3 11

//多播发现端口
#define SPDP_WELL_KNOWN_MULTICAST_PORT(ulDomainId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (unsigned long)ulDomainId + ADDITIONAL_OFFSETS_D0
//单播发现端口
#define SPDP_WELL_KNOWN_UNICAST_PORT(ulDomainId, ulParticipantId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (unsigned long)ulDomainId + ADDITIONAL_OFFSETS_D1 + \
    PARTICIPANTID_GAIN * (unsigned long)ulParticipantId
//用户多播端口
#define USER_DEFAULT_MULTICAST_PORT(ulDomainId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (unsigned long)ulDomainId + ADDITIONAL_OFFSETS_D2
//用户单播端口
#define USER_DEFAULT_UNICAST_PORT(ulDomainId, ulParticipantId) \
    PORT_BASE_NUMBER + DOMAINID_GAIN * (unsigned long)ulDomainId + ADDITIONAL_OFFSETS_D3 + \
    PARTICIPANTID_GAIN * (unsigned long)ulParticipantId


AGIDDS_NAMESPACE_END
