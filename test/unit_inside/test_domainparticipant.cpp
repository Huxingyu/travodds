#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
USING_TRAVODDS_NAMESPACE

/****************************** DomainParticipant *********************************************/
TEST(DomainParticipantTest, DomainParticipant_get_qos) {
    DomainParticipantQos setQos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, setQos, nullptr, STATUS_MASK_NONE);
    DomainParticipantQos getQos;
    ReturnCode_t result = par->get_qos(getQos);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(getQos.entity_factory.autoenable_created_entities, setQos.entity_factory.autoenable_created_entities);
    EXPECT_EQ(getQos.user_data.value, setQos.user_data.value);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipantQos setQos;
    setQos.user_data.value.push_back('A');
    setQos.user_data.value.push_back('N');
    ReturnCode_t result = par->set_qos(setQos);
    EXPECT_EQ(result, RETCODE_OK);
    DomainParticipantQos getQos;
    ReturnCode_t resultGET = par->get_qos(getQos);
    EXPECT_EQ(resultGET, RETCODE_OK);
    EXPECT_EQ(setQos.entity_factory.autoenable_created_entities, getQos.entity_factory.autoenable_created_entities);
    EXPECT_EQ(setQos.user_data.value, getQos.user_data.value);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_get_listener1) {
    DomainParticipantListener* listener = new DomainParticipantListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, listener, STATUS_MASK_NONE);
    const DomainParticipantListener* result = par->get_listener();
    EXPECT_EQ(result, listener);
    TheParticipantFactory->delete_participant(par);
    delete listener;
}
TEST(DomainParticipantTest, DomainParticipant_get_listener2) {
    DomainParticipantListener* listener = new DomainParticipantListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->set_listener(listener, STATUS_MASK_NONE);
    const DomainParticipantListener* result = par->get_listener();
    EXPECT_EQ(result, listener);
    TheParticipantFactory->delete_participant(par);
	delete listener;
}
TEST(DomainParticipantTest, DomainParticipant_set_listener1) {
    DomainParticipantListener* listener = new DomainParticipantListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t ret = par->set_listener(listener, STATUS_MASK_NONE);
    EXPECT_EQ(ret, RETCODE_OK);
    const DomainParticipantListener* listenerGet = par->get_listener();
    EXPECT_EQ(listenerGet, listener);
    TheParticipantFactory->delete_participant(par);
    delete listener;
}

TEST(DomainParticipantTest, DomainParticipant_create_delete_publisher) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(pub, nullptr);
    ReturnCode_t ret = par->delete_publisher(pub);
    EXPECT_EQ(pub, nullptr);
    EXPECT_EQ(ret, RETCODE_OK);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_publisher1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* pub = nullptr;
    ReturnCode_t result = par->delete_publisher(pub);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_publisher2) {

    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par_->delete_publisher(pub);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_delete_publisher3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = pub->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par->delete_publisher(pub);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_delete_subscriber) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(sub, nullptr);
    ReturnCode_t result = par->delete_subscriber(sub);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(sub, nullptr);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_subscriber1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = nullptr;
    ReturnCode_t result = par->delete_subscriber(sub);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_subscriber2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par_->delete_subscriber(sub);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_delete_subscriber3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    ReturnCode_t result = par->delete_subscriber(sub);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_topic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Topic* result = par->create_topic("topicname", "type_name", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_topic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    TopicQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = 0;
    Topic* result = par->create_topic("topicname", type_name, qos, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_topic3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Topic* result = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_delete_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    ReturnCode_t ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(top, nullptr);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_topic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Topic* top = nullptr;
    ReturnCode_t result = par->delete_topic(top);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_topic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par_->delete_topic(topic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_delete_topic3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par->delete_topic(topic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_contentfilteredtopic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* result = par->create_contentfilteredtopic("name", nullptr, filter_expression, expression_parameters);
    EXPECT_EQ(result, nullptr);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_contentfilteredtopic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* result = par_->create_contentfilteredtopic("name", topic, filter_expression, expression_parameters);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_create_contentfilteredtopic3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* result = par_->create_contentfilteredtopic("topicname", topic, filter_expression, expression_parameters);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_create_delete_contentfilteredtopic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("Topic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftopic = par->create_contentfilteredtopic("ContentFilteredTopic", related_topic, filter_expression, expression_parameters);
    EXPECT_NE(cftopic, nullptr);
    ReturnCode_t result = par->delete_contentfilteredtopic(cftopic);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(cftopic, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_contentfilteredtopic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ContentFilteredTopic* cftopic = nullptr;
    ReturnCode_t result = par->delete_contentfilteredtopic(cftopic);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_contentfilteredtopic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("Topic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftopic = par->create_contentfilteredtopic("ContentFilteredTopic", related_topic, filter_expression, expression_parameters);
    ReturnCode_t result = par_->delete_contentfilteredtopic(cftopic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_delete_contentfilteredtopic3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("Topic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftopic = par->create_contentfilteredtopic("ContentFilteredTopic", related_topic, filter_expression, expression_parameters);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = sub->create_datareader(cftopic, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = par->delete_contentfilteredtopic(cftopic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_multitopic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    MultiTopic* result = par->create_multitopic("name", "data", filter_expression, expression_parameters);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_multitopic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("name", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    StringSeq expression_parameters;
    MultiTopic* result = par->create_multitopic("name", type_name, "expression", expression_parameters);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_create_delete_multitopic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq expression_parameters;
    MultiTopic* mtopic = par->create_multitopic("mutopic", type_name, "expression", expression_parameters);
    EXPECT_NE(mtopic, nullptr);
    ReturnCode_t ret = par->delete_multitopic(mtopic);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(mtopic, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_multitopic1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    MultiTopic* mtopic = nullptr;
    ReturnCode_t result = par->delete_multitopic(mtopic);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_delete_multitopic2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq expression_parameters;
    MultiTopic* mtopic = par->create_multitopic("mutopic", type_name, "expression", expression_parameters);
    ReturnCode_t result = par_->delete_multitopic(mtopic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_delete_multitopic3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq expression_parameters;
    MultiTopic* mtopic = par->create_multitopic("mutopic", type_name, "expression", expression_parameters);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = sub->create_datareader(mtopic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = par->delete_multitopic(mtopic);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_find_topic) {

    Duration_t timeout{ 3,0 };
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* result = nullptr;
    std::thread th_1(
        [par, timeout, &result] {
            result = par->find_topic("topicFIND", timeout);

        });

    std::thread th_2(
        [par] {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            par->create_topic("topicFIND", TestModule::BasicDataTypeSupport::get_instance()->get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

        });
    th_1.join();
    th_2.join();
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DomainParticipantTest, DomainParticipant_find_topic1) {

    Duration_t timeout{ 3,0 };
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());

    Topic* result = nullptr;
    std::thread th_1(
        [par, timeout, &result] {
            result = par->find_topic("topicFIND", timeout);

        });

    std::thread th_2(
        [par] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            par->create_topic("topicFIND", TestModule::BasicDataTypeSupport::get_instance()->get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

        });
    th_1.join();
    th_2.join();

    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_find_topic2) {

    Duration_t timeout{ 3,0 };
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());

    Topic* result = nullptr;
    std::thread th_1(
        [par, timeout, &result] {
            result = par->find_topic("topicFIND", timeout);

        });

    std::thread th_2(
        [par] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            StringSeq expression_parameters;

            par->create_multitopic("topicFIND", TestModule::BasicDataTypeSupport::get_instance()->get_typename(), "subscription_expression", expression_parameters);

        });
    th_1.join();
    th_2.join();
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_lookup_topicdescription) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    TopicDescription* result = par->lookup_topicdescription("topic");
    EXPECT_EQ(result, nullptr);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_lookup_topicdescription1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("Topic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    TopicDescription* result = par->lookup_topicdescription("Topic");
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_get_domain_id) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainId_t result = par->get_domain_id();
    EXPECT_EQ(result, 0);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_set_default_topic_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    TopicQos qos;
    qos.history.depth = -1;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    ReturnCode_t result = par->set_default_topic_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_set_get_default_topic_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    TopicQos qos;
    ReturnCode_t result = par->set_default_topic_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);
    TopicQos qosGet;
    ReturnCode_t resultGet = par->get_default_topic_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.topic_data.value, qos.topic_data.value);
    EXPECT_EQ(qosGet.durability.kind, qos.durability.kind);
    EXPECT_EQ(qosGet.durability_service.service_cleanup_delay, qos.durability_service.service_cleanup_delay);
    EXPECT_EQ(qosGet.durability_service.history_kind, qos.durability_service.history_kind);
    EXPECT_EQ(qosGet.durability_service.history_depth, qos.durability_service.history_depth);
    EXPECT_EQ(qosGet.durability_service.max_samples, qos.durability_service.max_samples);
    EXPECT_EQ(qosGet.durability_service.max_instances, qos.durability_service.max_instances);
    EXPECT_EQ(qosGet.durability_service.max_samples_per_instance, qos.durability_service.max_samples_per_instance);
    EXPECT_EQ(qosGet.deadline.period, qos.deadline.period);
    EXPECT_EQ(qosGet.latency_budget.duration, qos.latency_budget.duration);
    EXPECT_EQ(qosGet.liveliness.kind, qos.liveliness.kind);
    EXPECT_EQ(qosGet.liveliness.lease_duration, qos.liveliness.lease_duration);
    EXPECT_EQ(qosGet.reliability.kind, qos.reliability.kind);
    EXPECT_EQ(qosGet.reliability.max_blocking_time, qos.reliability.max_blocking_time);
    EXPECT_EQ(qosGet.destination_order.kind, qos.destination_order.kind);
    EXPECT_EQ(qosGet.history.kind, qos.history.kind);
    EXPECT_EQ(qosGet.history.depth, qos.history.depth);
    EXPECT_EQ(qosGet.resource_limits.max_samples, qos.resource_limits.max_samples);
    EXPECT_EQ(qosGet.resource_limits.max_instances, qos.resource_limits.max_instances);
    EXPECT_EQ(qosGet.resource_limits.max_samples_per_instance, qos.resource_limits.max_samples_per_instance);
    EXPECT_EQ(qosGet.transport_priority.value, qos.transport_priority.value);
    EXPECT_EQ(qosGet.lifespan.duration, qos.lifespan.duration);
    EXPECT_EQ(qosGet.ownership.kind, qos.ownership.kind);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_contains_entity) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    InstanceHandle_t handel = topic->get_instance_handle();
    bool ret = par->contains_entity(handel);
    EXPECT_EQ(ret, true);
    ret = par_->contains_entity(handel);
    EXPECT_EQ(ret, false);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    handel = pub->get_instance_handle();
    ret = par->contains_entity(handel);
    EXPECT_EQ(ret, true);
    ret = par_->contains_entity(handel);
    EXPECT_EQ(ret, false);
    DataWriter* dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    handel = dw->get_instance_handle();
    ret = par->contains_entity(handel);
    EXPECT_EQ(ret, true);
    ret = par_->contains_entity(handel);
    EXPECT_EQ(ret, false);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    handel = sub->get_instance_handle();
    ret = par->contains_entity(handel);
    EXPECT_EQ(ret, true);
    ret = par_->contains_entity(handel);
    EXPECT_EQ(ret, false);
    DataReader* dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    handel = dr->get_instance_handle();
    ret = par->contains_entity(handel);
    EXPECT_EQ(ret, true);
    ret = par_->contains_entity(handel);
    EXPECT_EQ(ret, false);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par_);
}
TEST(DomainParticipantTest, DomainParticipant_registe_type) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    ReturnCode_t result = par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    EXPECT_EQ(result, RETCODE_OK);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_registe_type1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = par->registe_type("", TestModule::BasicDataTypeSupport::get_instance());
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    TheParticipantFactory->delete_participant(par);
}
TEST(DomainParticipantTest, DomainParticipant_registe_type2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    ReturnCode_t result = par->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());;
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    TheParticipantFactory->delete_participant(par);
}
