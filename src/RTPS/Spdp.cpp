#include "RTPS/Spdp.h"

#include "DDS/DataWriterImpl.h"
#include "DDS/DomainParticipant.h"
#include "DDS/Publisher.h"

USING_AGIDDS_NAMESPACE

Spdp::Spdp(Participant* participant)
{
	DataWriterQos writerQos;
	writerQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;


}

void Spdp::SetParticipantDataWriter(DataWriterImpl * dataWriter)
{
	DataWriterQos qos;
	dataWriter->get_qos(qos);
	//std::shared_ptr<Writer> writer = participant_->CreateWriter(qos, ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER);
	//writer_ = dynamic_cast<StatelessWriter*>(writer.get());

	//TODO 周期修改为可灵活配置的
	Duration_t resendPeriod = { 30,0 };
	writer_ = new StatelessWriter(qos, resendPeriod);

	//TODO 补充locator与ip的装换函数
	Locator_t locator;
	locator.kind = LOCATOR_KIND_UDPv4;
	locator.port = SPDP_WELL_KNOWN_MULTICAST_PORT(dataWriter->get_publisher()->get_participant()->get_domain_id());
	locator.address[12] = 0xEF;
	locator.address[13] = 0xFF;
	locator.address[14] = 0x00;
	locator.address[14] = 0x01;

	writer_->ReaderLocatorAdd(locator);

	//TODO DataWriter与writer关联
}

void Spdp::SetParticipantDataReqader(DataReaderImpl * dataReader)
{
	DataReaderQos qos;
	dataReader->get_qos(qos);
	std::shared_ptr<Reader> reader = participant_->CreateReader(qos, ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER);
	reader_ = dynamic_cast<StatefulReader*>(reader.get());

	//TODO DataReader与reader关联
}

void Spdp::on_data_available()
{

}

void Spdp::ReceiveSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData & data)
{
	
	if (0 == remoteParticipantData_.count(data.ddsParticipantData.key)) {
		remoteParticipantData_[data.ddsParticipantData.key] = data;
	}

	//TODO 为内置端点配置代理
}
