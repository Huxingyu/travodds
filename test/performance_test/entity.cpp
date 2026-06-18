#include "entity.h"
#include "dcps/condition/waitset.h"
#include "typetypesupport.h"

PerformanceTest::TestEntity::TestEntity(const TestParameter& param)
: param_(param)
{
	DomainParticipantQos qos;
	TheParticipantFactory->get_default_participant_qos(qos);
	std::string name = param_.testType == TestType::DELAY ? "TIME_DELAY_TEST" : "THROUGHPUT_TEST";
	qos.user_data.value.resize(name.size());
	qos.user_data.value.assign(name.begin(),name.end());
	participant_ = TheParticipantFactory->create_participant(param_.domainID, qos, nullptr, STATUS_MASK_NONE);

	const char* type_name = PerformanceTest::TestDataTypeSupport::get_instance()->get_typename();
	participant_->registe_type(
		type_name,
		PerformanceTest::TestDataTypeSupport::get_instance()
		);

	std::string dw_topic_name = param_.entityType == EntityType::PUB ? name + "_1" : name + "_2";
	Topic* dw_topic = participant_->create_topic(dw_topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	writer_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, PUBLICATION_MATCHED_STATUS)
	    ->create_datawriter(dw_topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	std::string dr_topic_name = param_.entityType == EntityType::SUB ? name + "_1" : name + "_2";
	Topic* dr_topic = participant_->create_topic(dr_topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	reader_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, SUBSCRIPTION_MATCHED_STATUS)
		->create_datareader(dr_topic, DATAREADER_QOS_DEFAULT, this, DATA_AVAILABLE_STATUS);
}

PerformanceTest::TestEntity::~TestEntity()
{
	participant_->delete_contained_entities();
	TheParticipantFactory->delete_participant(participant_);
}

void PerformanceTest::TestEntity::wait_match()
{
	StatusCondition& dr_sc = const_cast<Subscriber*>(reader_->get_subscriber())->get_statuscondition();
	StatusCondition& dw_sc = const_cast<Publisher*>(writer_->get_publisher())->get_statuscondition();

	WaitSet ws;
	ws.attach_condition(dr_sc);
	ws.attach_condition(dw_sc);

	ConditionSeq active_conditions;

	ws.wait(active_conditions, { 10,0 });

	if (active_conditions.size() == 1) {
		ws.detach_condition(*active_conditions[0]);
		ws.wait(active_conditions, { 10,0 });
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
