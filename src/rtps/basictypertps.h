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

#include "type/guid.h"

TRAVODDS_NAMESPACE_BEGIN

// rtps entity attribute



using VendorId_t = std::array<octet, 2>;//octet[2];

const VendorId_t VENDORID_UNKNOWN = { 0,0 };
const VendorId_t VENDORID_PROSIMA = { 0,0 };

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
	Parameter_t() : parameterId(0), length(0) {
	}
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
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER   0x00000001 << 10
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER   0x00000001 << 11

#ifdef DDSSECURITY
/* 安全相关扩展 */
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER        0x00000001 << 16
#define DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR         0x00000001 << 17
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER       0x00000001 << 18
#define DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR        0x00000001 << 19
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_WRITER   0x00000001 << 20
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_READER   0x00000001 << 21
#define BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_WRITER   0x00000001 << 22
#define BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_READER   0x00000001 << 23
#define BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_WRITER   0x00000001 << 24
#define BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_READER   0x00000001 << 25

#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_ANNOUNCER       (0x00000001 << 26)
#define DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_DETECTOR        (0x00000001 << 27)
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
