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

#include "RTPS/RTPSEntity.h"
#include "RTPS/Reader.h"

AGIDDS_NAMESPACE_BEGIN

class Writer;
class Reader;
class DomainParticipantImpl;
class DataWriterImpl;
class DataReaderImpl;

class Participant: public RTPSEntity {
public:
	Participant(std::shared_ptr<DomainParticipantImpl>& domainImpl);

	Participant(const Participant&) = delete ;

	virtual ~Participant();

	int Init();

	std::shared_ptr<Writer> CreateWriter(/*std::shared_ptr<DataWriterImpl>& dw,*/const DataWriterQos& dwQos, const EntityId_t& EntityID);

	std::shared_ptr<Reader> CreateReader(/*std::shared_ptr<DataReaderImpl>& dr,*/const DataReaderQos& drQos, const EntityId_t& EntityID);

	int DeleteWriter(std::shared_ptr<Writer> writer);

	int DeleteReader(std::shared_ptr<Reader> reader);

	int AddRemoteWriter(GUID_t guid, LocatorSeq & unicastLocatorList, LocatorSeq & multicastLocatorList,PublicationBuiltinTopicData & data);

	int AddRemoteReader(GUID_t guid, LocatorSeq & unicastLocatorList, LocatorSeq & multicastLocatorList,SubscriptionBuiltinTopicData & data);

	int RemoveRemoteWriter(GUID_t guid);

	int RemoveRemoteReader(GUID_t guid);

private:
	Locator_t* defaultUnicastLocatorList;
	Locator_t* defaultMulticastLocatorList;
	ProtocolVersion_t protocolVersion_;
	VendorId_t vendorId_;

	unsigned int readerId_;
	unsigned int writerId_;

	std::shared_ptr<DomainParticipantImpl> domainImpl_;
};

AGIDDS_NAMESPACE_END
