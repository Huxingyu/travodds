#ifdef TEST_DOMAINPARTICIPANTFACTORY
#include "test_head.hpp"
/****************************** DomainParticipantFactory *********************************************/
TEST(DDSTest, DomainParticipantFactory_create_delete_participant) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    EXPECT_NE(par, nullptr);
    ReturnCode_t result = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(result, RETCODE_OK);
}
TEST(DDSTest, DomainParticipantFactory_delete_participant1) {
    DomainParticipant* par = nullptr;
    ReturnCode_t result = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
}
TEST(DDSTest, DomainParticipantFactory_delete_participant2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DomainParticipantFactory_get_instance) {
    DomainParticipantFactory* result = TheParticipantFactory->get_instance();
    EXPECT_NE(result, nullptr);
}
TEST(DDSTest, DomainParticipantFactory_lookup_participant1) {

    DomainParticipant* result = TheParticipantFactory->lookup_participant(0);
    EXPECT_EQ(result, nullptr);
}
TEST(DDSTest, DomainParticipantFactory_lookup_participant2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    DomainParticipant* result = TheParticipantFactory->lookup_participant(0);
    EXPECT_EQ(result, par);
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DomainParticipantFactory_set_get_default_participant_qos) {
    DomainParticipantQos qos;
    ReturnCode_t result = TheParticipantFactory->set_default_participant_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    DomainParticipantQos qosGet;
    ReturnCode_t resultGet = TheParticipantFactory->get_default_participant_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.user_data.value, qos.user_data.value);
    EXPECT_EQ(qosGet.entity_factory.autoenable_created_entities, qos.entity_factory.autoenable_created_entities);
}

TEST(DDSTest, DomainParticipantFactory_set_get_qos) {
    DomainParticipantFactoryQos qos;
    ReturnCode_t result = TheParticipantFactory->set_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    DomainParticipantFactoryQos qosGet;
    ReturnCode_t resultGet = TheParticipantFactory->get_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.entity_factory.autoenable_created_entities, qos.entity_factory.autoenable_created_entities);
}

#endif // TEST_DOMAINPARTICIPANTFACTORY
