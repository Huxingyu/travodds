#include "RTPS/StatefulReader.h"
USING_AGIDDS_NAMESPACE

void StatefulReader::MatchedWriterAdd(WriterProxy * writerProxy)
{
	matchedWriters_[writerProxy->GetGuid()] = writerProxy;
}

void StatefulReader::MatchedWriterRemove(WriterProxy * writerProxy)
{
	matchedWriters_.erase(writerProxy->GetGuid());
}

WriterProxy * StatefulReader::MatchedWriterLookup(GUID_t writerGuid)
{
	if(0 == matchedWriters_.count(writerGuid))
		return nullptr;

	return matchedWriters_[writerGuid];
}
