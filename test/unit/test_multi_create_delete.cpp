#ifdef TEST_MULTI_CREATE_DELETE

#include "test_head.hpp"
TEST(DDSTest, DomainParticipantFactory_multi_create_delete_participant) {
    //std::vector<std::shared_ptr<std::thread>> vec;
    //std::atomic_bool flag(false);
    for (uint32_t i = 0; i < 1000; i++) {
        //    std::shared_ptr<std::thread> th = std::make_shared<std::thread>([&flag]() {
        //        while (!flag.load()) {
        //            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //        }
        DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(par, nullptr);
        ReturnCode_t result = TheParticipantFactory->delete_participant(par);
        EXPECT_EQ(result, RETCODE_OK);
        //        });
        //    vec.push_back(th);
    }
    //flag.store(true);

    //for (auto& it : vec) {
    //    if (it->joinable()) {
    //        it->join();
    //    }
    //}
}
TEST(DDSTest, DomainParticipantFactory_multi_create_delete_participant_1) {
    std::vector<DomainParticipant*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(par, nullptr);
        vec.push_back(par);

    }
    for (auto it : vec) {
        ReturnCode_t result = TheParticipantFactory->delete_participant(it);
        EXPECT_EQ(result, RETCODE_OK);
    }
}
#endif
#if 0
TEST(DDSTest, DomainParticipant_multi_create_delete_publisher) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(pub, nullptr);
        ReturnCode_t ret = par->delete_publisher(pub);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DomainParticipant_multi_create_delete_publisher_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    std::vector<Publisher*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(pub, nullptr);
        vec.push_back(pub);
    }
    for (auto it : vec) {
        ReturnCode_t ret = par->delete_publisher(it);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
TEST(DDSTest, DomainParticipant_multi_create_delete_publisher_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(pub, nullptr);
    }
    ReturnCode_t ret = par->delete_contained_entities();
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
TEST(DDSTest, DomainParticipant_multi_create_delete_subscriber) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(sub, nullptr);
        ReturnCode_t ret = par->delete_subscriber(sub);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, DomainParticipant_multi_create_delete_subscriber_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    std::vector<Subscriber*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(sub, nullptr);
        vec.push_back(sub);
    }
    for (auto it : vec) {
        ReturnCode_t ret = par->delete_subscriber(it);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, DomainParticipant_multi_create_delete_subscriber_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(sub, nullptr);
    }
    ReturnCode_t ret = par->delete_contained_entities();
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
TEST(DDSTest, DomainParticipant_multi_create_delete_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);

    for (uint32_t i = 0; i < 1000; i++) {
        std::string name = "topic_" + std::to_string(i);
        Topic* top = par->create_topic(name, "data", TOPIC_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(top, nullptr);
        ReturnCode_t ret = par->delete_topic(top);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, DomainParticipant_multi_create_delete_topic_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    std::vector<Topic*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        std::string name = "topic_" + std::to_string(i);
        Topic* top = par->create_topic(name, "data", TOPIC_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(top, nullptr);
        vec.push_back(top);
    }
    for (auto it : vec) {
        ReturnCode_t ret = par->delete_topic(it);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, DomainParticipant_multi_create_delete_topic_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    for (uint32_t i = 0; i < 1000; i++) {
        std::string name = "topic_" + std::to_string(i);
        Topic* top = par->create_topic(name, "data", TOPIC_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(top, nullptr);
    }
    ReturnCode_t ret = par->delete_contained_entities();
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
TEST(DDSTest, Subscriber_multi_create_delete_datareader) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topic", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dr, nullptr);
        ReturnCode_t ret = sub->delete_datareader(dr);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = par->delete_subscriber(sub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Subscriber_multi_create_delete_datareader_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topic", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    std::vector<DataReader*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dr, nullptr);
        vec.push_back(dr);
    }
    for (auto it : vec) {
        ReturnCode_t ret = sub->delete_datareader(it);
        EXPECT_EQ(ret, RETCODE_OK);
    }

    ReturnCode_t ret = par->delete_subscriber(sub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Subscriber_multi_create_delete_datareader_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topic", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dr, nullptr);
    }
    ReturnCode_t ret = sub->delete_contained_entities();
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_subscriber(sub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
TEST(DDSTest, Publisher_multi_create_delete_datawriter) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        DataWriter* dw = pub->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dw, nullptr);
        ReturnCode_t ret = pub->delete_datawriter(dw);
        EXPECT_EQ(ret, RETCODE_OK);
    }
    ReturnCode_t ret = par->delete_publisher(pub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Publisher_multi_create_delete_datawriter_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    std::vector<DataWriter*> vec;
    for (uint32_t i = 0; i < 1000; i++) {
        DataWriter* dw = pub->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dw, nullptr);
        vec.push_back(dw);
    }
    for (auto it : vec) {
        ReturnCode_t ret = pub->delete_datawriter(it);
        EXPECT_EQ(ret, RETCODE_OK);
    }

    ReturnCode_t ret = par->delete_publisher(pub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}

TEST(DDSTest, Publisher_multi_create_delete_datawriter_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    for (uint32_t i = 0; i < 1000; i++) {
        DataWriter* dw = pub->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
        EXPECT_NE(dw, nullptr);
    }
    ReturnCode_t ret = pub->delete_contained_entities();
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_publisher(pub);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = par->delete_topic(top);
    EXPECT_EQ(ret, RETCODE_OK);
    ret = TheParticipantFactory->delete_participant(par);
    EXPECT_EQ(ret, RETCODE_OK);
}
#endif // TEST_MULTI_CREATE_DELETE
