#include "Sedp.h"

USING_AGIDDS_NAMESPACE

void Sedp::AddSedpProxy(SPDPdiscoveredParticipantData & data)
{
	GUID_t guid;
	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	ReaderProxy* readerProxy = new ReaderProxy(guid,data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	publicationsWriter_->MatchedReaderAdd(readerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	readerProxy = new ReaderProxy(guid, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	subscriptionsWriter_->MatchedReaderAdd(readerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	WriterProxy* writerProxy = new WriterProxy(guid, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	publicationsReader_->MatchedWriterAdd(writerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	writerProxy = new WriterProxy(guid, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	subscriptionsReader_->MatchedWriterAdd(writerProxy);
}

void Sedp::RemoveSedpProxy(SPDPdiscoveredParticipantData & data)
{
	GUID_t guid;
	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	ReaderProxy* readerProxy = publicationsWriter_->MatchedReaderLookup(guid);
	publicationsWriter_->MatchedReaderRemove(readerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	readerProxy = subscriptionsWriter_->MatchedReaderLookup(guid);
	subscriptionsWriter_->MatchedReaderRemove(readerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	WriterProxy* writerProxy = publicationsReader_->MatchedWriterLookup(guid);
	publicationsReader_->MatchedWriterRemove(writerProxy);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	writerProxy = subscriptionsReader_->MatchedWriterLookup(guid);
	subscriptionsReader_->MatchedWriterRemove(writerProxy);
}
