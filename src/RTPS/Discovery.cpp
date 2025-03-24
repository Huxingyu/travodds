#include "RTPS/Discovery.h"

#include "DDS/DomainParticipant.h"

USING_AGIDDS_NAMESPACE

Discovery::Discovery()
{

}

void Discovery::InitBuiltinEndpoint()
{
	builtinPublisher_ = new PublisherImpl(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, domainParticipant_);
	builtinSubScriber_ = new SubscriberImpl(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, domainParticipant_);

	DCPSParticipant_ = new TopicImpl("DCPSParticipant", "SPDPdiscoveredParticipantData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, domainParticipant_);

	//TODO ≤π≥‰æﬂÃÂTypeSupport
	DataWriterQos writerQoS;
	builtinParticipantDataWriter_ = new DataWriterImpl(DCPSParticipant_, writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_, nullptr);

	DataReaderQos readerQos;
	builtinParticipantDataReader_ = new DataReaderImpl(DCPSParticipant_, readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_, nullptr);

}
