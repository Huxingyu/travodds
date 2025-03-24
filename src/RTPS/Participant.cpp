#include "RTPS/Participant.h"

USING_AGIDDS_NAMESPACE

std::shared_ptr<Writer> Participant::CreateWriter(const DataWriterQos & dwQos, const EntityId_t & EntityID)
{
	return std::shared_ptr<Writer>();
}

std::shared_ptr<Reader> Participant::CreateReader(const DataReaderQos & drQos, const EntityId_t & EntityID)
{
	return std::shared_ptr<Reader>();
}
