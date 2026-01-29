#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/statuscondition.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE


TEST(ConditionTest, Condition_statusCondition)
{
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, DATA_ON_READERS_STATUS);
	StatusCondition& sc = par->get_statuscondition();
	EXPECT_FALSE(sc.get_trigger_value());
	TheParticipantFactory->delete_participant(par);

}


