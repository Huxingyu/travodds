#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/waitset.h"
#include "testdataTypeSupport.h"
USING_TRAVODDS_NAMESPACE

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_filter_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = 10 AND name = 'abc'";
	StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    EXPECT_NE(cftop, nullptr);
    std::string result = cftop->get_filter_expression();
    EXPECT_EQ(result, filter_expression);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_related_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    Topic* result = cftop->get_related_topic();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = %0 AND name = 'abc'";
    StringSeq expression_parameters;
	expression_parameters.push_back("10");
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    StringSeq seqGet;
    ReturnCode_t result = cftop->get_expression_parameters(seqGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(seqGet, expression_parameters);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_set_get_expression_parameters) {
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "id = %0 AND name = 'abc'";
    StringSeq expression_parameters = { "10" };
    ContentFilteredTopic* cftop = par_r->create_contentfilteredtopic("CFT_ContentFilteredTopic", top_r, filter_expression, expression_parameters);
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(cftop, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

    StatusCondition& sc = dr->get_statuscondition();
    WaitSet ws;
    ws.attach_condition(sc);
    ConditionSeq active_conditions;
    std::thread th_1(
        [&] {
            EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

            SequenceData<TestModule::BasicData> data_seq;
            SampleInfoSeq info_seq;

            EXPECT_EQ(dr->take(
                data_seq, info_seq, LENGTH_UNLIMITED,
                ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
                RETCODE_OK);
            EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_TIMEOUT);
        });
    std::thread th_2(
        [&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            TestModule::BasicData data;
            data.id = 10;
            data.name = "abc";
            EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
            StringSeq parameters = { "20" };
            std::this_thread::sleep_for(std::chrono::seconds(1));
            EXPECT_EQ(cftop->set_expression_parameters(parameters), RETCODE_OK);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);

        });
    th_1.join();
    th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_fuctest1) {
    const char* type_name = TestModule::FilterableDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	std::string filter_expression = "((shortField > %0 AND longField < %1) OR (floatField BETWEEN %2 AND %3))\
    AND(stringField LIKE %4 OR fixedStringField = %5)\
    AND(nestedField.nestedFlag = 1 OR structArray[0].nestedId = %6)\
    AND NOT(enumField = 'VALUE_E')";
	StringSeq expression_parameters;
	expression_parameters.push_back("10");
	expression_parameters.push_back("1000");
	expression_parameters.push_back("1.5");
	expression_parameters.push_back("5.5");
	expression_parameters.push_back("'test%'");
	expression_parameters.push_back("'fixedValue'");
	expression_parameters.push_back("42");
	ContentFilteredTopic* cftop = par_r->create_contentfilteredtopic("CFT_ContentFilteredTopic", top_r, filter_expression, expression_parameters);
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(cftop, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

    std::vector<TestModule::FilterableData> datas;

    // ====== 数据1：正确的数据，应该匹配过滤条件 ======
    {
        TestModule::FilterableData data1;
        // 条件1: shortField > 10 AND longField < 1000
        data1.shortField = 50;      // > 10
        data1.longField = 500;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data1.floatField = 3.0f;    // 在1.5-5.5之间

        // 条件3: stringField LIKE 'test%' OR fixedStringField = 'fixedValue'
        data1.stringField = "testData";        // 匹配 'test%'
        data1.fixedStringField = "otherValue"; // 不匹配，但stringField匹配

        // 条件4: nestedField.nestedFlag = TRUE OR structArray[0].nestedId = 42
        data1.nestedField.nestedFlag = true;   // 匹配 TRUE
        data1.structArray[0].nestedId = 100;   // 不匹配，但nestedFlag匹配

        // 条件5: NOT(enumField = 'VALUE_E')
        data1.enumField = TestModule::VALUE_A; // 不是VALUE_E

        datas.push_back(data1);
    }

    // ====== 数据2：正确的数据，从不同路径匹配过滤条件 ======
    {
        TestModule::FilterableData data2;
        // 条件1: shortField > 10 AND longField < 1000
        data2.shortField = 100;     // > 10
        data2.longField = 800;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data2.floatField = 2.0f;    // 在1.5-5.5之间

        // 条件3: stringField LIKE 'test%' OR fixedStringField = 'fixedValue'
        data2.stringField = "otherData";    // 不匹配 'test%'
        data2.fixedStringField = "fixedValue"; // 匹配 'fixedValue'

        // 条件4: nestedField.nestedFlag = TRUE OR structArray[0].nestedId = 42
        data2.nestedField.nestedFlag = false;  // 不匹配
        data2.structArray[0].nestedId = 42;    // 匹配 42

        // 条件5: NOT(enumField = 'VALUE_E')
        data2.enumField = TestModule::VALUE_B; // 不是VALUE_E

        datas.push_back(data2);
    }

    // ====== 数据3：错误数据 - floatField不符合BETWEEN条件 ======
    {
        TestModule::FilterableData data3;
        // 条件1: shortField > 10 AND longField < 1000
        data3.shortField = 5;      // > 10
        data3.longField = 1500;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data3.floatField = 0.6f;    // 错误：不在1.5-5.5之间

        // OR部分可能满足，但AND要求所有条件都满足
        data3.stringField = "testData";
        data3.fixedStringField = "fixedValue";
        data3.nestedField.nestedFlag = true;
        data3.structArray[0].nestedId = 42;
        data3.enumField = TestModule::VALUE_A;

        datas.push_back(data3);
    }

    // ====== 数据4：错误数据 - 字符串条件都不满足 ======
    {
        TestModule::FilterableData data4;
        // 条件1: shortField > 10 AND longField < 1000
        data4.shortField = 50;      // > 10
        data4.longField = 500;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data4.floatField = 3.0f;    // 在1.5-5.5之间

        // 条件3: stringField LIKE 'test%' OR fixedStringField = 'fixedValue'
        data4.stringField = "wrongData";        // 错误：不以'test'开头
        data4.fixedStringField = "wrongFixed";  // 错误：不等于'fixedValue'

        // 其他条件满足也没用，因为AND要求所有条件
        data4.nestedField.nestedFlag = true;
        data4.structArray[0].nestedId = 42;
        data4.enumField = TestModule::VALUE_A;

        datas.push_back(data4);
    }

    // ====== 数据5：错误数据 - 条件4排除 ======
    {
        TestModule::FilterableData data5;
        // 条件1: shortField > 10 AND longField < 1000
        data5.shortField = 50;      // > 10
        data5.longField = 500;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data5.floatField = 3.0f;    // 在1.5-5.5之间

        // 条件3: stringField LIKE 'test%' OR fixedStringField = 'fixedValue'
        data5.stringField = "testData";        // 匹配
        data5.fixedStringField = "fixedValue"; // 匹配

        // 条件4: nestedField.nestedFlag = TRUE OR structArray[0].nestedId = 42
        data5.nestedField.nestedFlag = false;   // 匹配
        data5.structArray[0].nestedId = 43;    // 匹配

        // 条件5: NOT(enumField = 'VALUE_E')
        data5.enumField = TestModule::VALUE_A;

        datas.push_back(data5);
    }

    // ====== 数据6：错误数据 - enumField为VALUE_E被NOT排除 ======
    {
        TestModule::FilterableData data6;
        // 条件1: shortField > 10 AND longField < 1000
        data6.shortField = 50;      // > 10
        data6.longField = 500;      // < 1000

        // 条件2: floatField BETWEEN 1.5 AND 5.5
        data6.floatField = 3.0f;    // 在1.5-5.5之间

        // 条件3: stringField LIKE 'test%' OR fixedStringField = 'fixedValue'
        data6.stringField = "testData";        // 匹配
        data6.fixedStringField = "fixedValue"; // 匹配

        // 条件4: nestedField.nestedFlag = TRUE OR structArray[0].nestedId = 42
        data6.nestedField.nestedFlag = true;   // 匹配
        data6.structArray[0].nestedId = 42;    // 匹配

        // 条件5: NOT(enumField = 'VALUE_E')
        data6.enumField = TestModule::VALUE_E; // 错误：等于VALUE_E，NOT排除

        datas.push_back(data6);
    }


    StatusCondition& sc = dr->get_statuscondition();
    WaitSet ws;
	ws.attach_condition(sc);
    ConditionSeq active_conditions;
    std::thread th_1(
        [&] {
            for (int i = 0; i < 2; i++) {
                EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

                SequenceData<TestModule::FilterableData> data_seq;
                SampleInfoSeq info_seq;

                EXPECT_EQ(dr->take(
                    data_seq, info_seq, LENGTH_UNLIMITED,
                    ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
                    RETCODE_OK);
                EXPECT_TRUE(data_seq[0].shortField > 10);
                EXPECT_TRUE(data_seq[0].longField < 1000 ||
                    (data_seq[0].floatField >= 1.5f && data_seq[0].floatField <= 5.5f));
                EXPECT_TRUE(data_seq[0].stringField.find("test") == 0 ||
                    data_seq[0].fixedStringField == "fixedValue");
                EXPECT_TRUE(data_seq[0].nestedField.nestedFlag == true ||
                    data_seq[0].structArray[0].nestedId == 42);
                EXPECT_NE(data_seq[0].enumField, TestModule::VALUE_E);
            }
            EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_TIMEOUT);
        });
    std::thread th_2(
        [&] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            for (const auto& data : datas) {
                EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    th_1.join();
    th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);
}


TEST(ContentFilteredTopicTest, ContentFilteredTopic_fuctest2) {
    const char* type_name = TestModule::FilterableDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "(longArray[0] > 10 OR doubleArray[0] < 0.0)\
    AND (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5)\
    AND (structArray[0].nestedArray[0] > 0 AND structArray[1].nestedArray[1] < 100)";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par_r->create_contentfilteredtopic("CFT_ContentFilteredTopic", top_r, filter_expression, expression_parameters);
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(cftop, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

    std::vector<TestModule::FilterableData> datas;
    // ====== 数据1：完全匹配所有条件 ======
    {
        TestModule::FilterableData data1;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)
        data1.longArray[0] = 20;    // > 10，满足第一个OR条件
        data1.doubleArray[0] = 1.0; // > 0.0，但不影响，因为OR已满足

        // 条件2: (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5)
        data1.matrix2x3[0][0] = 1;  // = 1，满足第二个OR条件
        data1.floatMatrix[1][1] = 0.3f; // < 0.5，但不影响

        // 条件3: (structArray[0].nestedArray[0] > 0 AND structArray[1].nestedArray[1] < 100)
        data1.structArray[0].nestedArray[0] = 50;  // > 0
        data1.structArray[1].nestedArray[1] = 80;  // < 100

        // 填充一些其他字段防止未初始化
        data1.shortField = 1;
        data1.stringField = "data1";
        data1.enumField = TestModule::VALUE_A;

        datas.push_back(data1);
    }

    // ====== 数据2：通过OR的其他路径匹配 ======
    {
        TestModule::FilterableData data2;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)
        data2.longArray[0] = 5;     // <= 10，不满足
        data2.doubleArray[0] = -0.5; // < 0.0，满足OR的另一个条件

        // 条件2: (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5)
        data2.matrix2x3[0][0] = 0;  // != 1，不满足
        data2.floatMatrix[1][1] = 0.8f; // > 0.5，满足OR的另一个条件

        // 条件3: (structArray[0].nestedArray[0] > 0 AND structArray[1].nestedArray[1] < 100)
        data2.structArray[0].nestedArray[0] = 30;  // > 0
        data2.structArray[1].nestedArray[1] = 50;  // < 100

        // 填充其他字段
        data2.shortField = 2;
        data2.stringField = "data2";
        data2.enumField = TestModule::VALUE_B;

        datas.push_back(data2);
    }

    // ====== 数据3：错误数据 - 条件1的OR都不满足 ======
    {
        TestModule::FilterableData data3;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)  - 都不满足
        data3.longArray[0] = 10;    // 错误：= 10，不是 >
        data3.doubleArray[0] = 0.0; // 错误：= 0.0，不是 <

        // 即使其他条件满足，整体AND也不满足
        data3.matrix2x3[0][0] = 1;  // = 1
        data3.floatMatrix[1][1] = 0.8f; // > 0.5
        data3.structArray[0].nestedArray[0] = 1;  // > 0
        data3.structArray[1].nestedArray[1] = 99; // < 100

        // 填充其他字段
        data3.shortField = 3;
        data3.stringField = "data3_wrong";
        data3.enumField = TestModule::VALUE_C;

        datas.push_back(data3);
    }

    // ====== 数据4：错误数据 - 条件2的OR都不满足 ======
    {
        TestModule::FilterableData data4;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)
        data4.longArray[0] = 15;    // > 10，满足

        // 条件2: (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5) - 都不满足
        data4.matrix2x3[0][0] = 2;  // 错误：≠ 1
        data4.floatMatrix[1][1] = 0.5f; // 错误：= 0.5，不是 >

        // 条件3即使满足也没用
        data4.structArray[0].nestedArray[0] = 1;  // > 0
        data4.structArray[1].nestedArray[1] = 99; // < 100

        // 填充其他字段
        data4.shortField = 4;
        data4.stringField = "data4_wrong";
        data4.enumField = TestModule::VALUE_D;

        datas.push_back(data4);
    }

    // ====== 数据5：错误数据 - 条件3的AND不满足（第一个条件失败） ======
    {
        TestModule::FilterableData data5;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)
        data5.longArray[0] = 20;    // > 10，满足

        // 条件2: (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5)
        data5.matrix2x3[0][0] = 1;  // = 1，满足

        // 条件3: (structArray[0].nestedArray[0] > 0 AND structArray[1].nestedArray[1] < 100)
        data5.structArray[0].nestedArray[0] = 0;  // 错误：= 0，不是 > 0
        data5.structArray[1].nestedArray[1] = 50; // < 100，但AND要求两个都满足

        // 填充其他字段
        data5.shortField = 5;
        data5.stringField = "data5_wrong";
        data5.enumField = TestModule::VALUE_E;

        datas.push_back(data5);
    }

    // ====== 数据6：错误数据 - 条件3的AND不满足（第二个条件失败） ======
    {
        TestModule::FilterableData data6;

        // 条件1: (longArray[0] > 10 OR doubleArray[0] < 0.0)
        data6.longArray[0] = 20;    // > 10，满足

        // 条件2: (matrix2x3[0][0] = 1 OR floatMatrix[1][1] > 0.5)
        data6.matrix2x3[0][0] = 1;  // = 1，满足

        // 条件3: (structArray[0].nestedArray[0] > 0 AND structArray[1].nestedArray[1] < 100)
        data6.structArray[0].nestedArray[0] = 10; // > 0，满足
        data6.structArray[1].nestedArray[1] = 100; // 错误：= 100，不是 < 100

        // 填充其他字段
        data6.shortField = 6;
        data6.stringField = "data6_wrong";
        data6.enumField = TestModule::VALUE_A;

        datas.push_back(data6);
    }

    StatusCondition& sc = dr->get_statuscondition();
    WaitSet ws;
    ws.attach_condition(sc);
    ConditionSeq active_conditions;
    std::thread th_1(
        [&] {
            for (int i = 0; i < 2; i++) {
                EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

                SequenceData<TestModule::FilterableData> data_seq;
                SampleInfoSeq info_seq;

                EXPECT_EQ(dr->take(
                    data_seq, info_seq, LENGTH_UNLIMITED,
                    ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
                    RETCODE_OK);
                EXPECT_TRUE(data_seq[0].longArray[0] > 10 ||
                    data_seq[0].doubleArray[0] < 0.0);
                EXPECT_TRUE(data_seq[0].matrix2x3[0][0] == 1 ||
                    data_seq[0].floatMatrix[1][1] > 0.5f);
                EXPECT_TRUE(data_seq[0].structArray[0].nestedArray[0] > 0 &&
                    data_seq[0].structArray[1].nestedArray[1] < 100);
            }
            EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_TIMEOUT);

        });
    std::thread th_2(
        [&] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            for (const auto& data : datas) {
                EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    th_1.join();
    th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);
    
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_fuctest3) {
    const char* type_name = TestModule::FilterableDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    std::string filter_expression = "(shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)\
        AND(longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)\
        AND(charField = 'A' OR byteField = 9)\
        AND(stringField LIKE 'test%' OR fixedStringField = 'fixed')\
        AND(enumField = 'VALUE_A' OR enumField = 'VALUE_B')\
        AND(boolArray[0] = 1 OR boolArray[1] = 0)";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par_r->create_contentfilteredtopic("CFT_ContentFilteredTopic", top_r, filter_expression, expression_parameters);
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(cftop, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

    std::vector<TestModule::FilterableData> datas;

    // ====== 数据1：完全匹配所有条件（每个OR的第一个条件） ======
    {
        TestModule::FilterableData data1;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data1.shortField = 1;        // = 1

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data1.longlongField = 5;     // = 5

        // 条件3: (charField = 'A' OR byteField = 9)
        data1.charField = 'A';       // = 'A'

        // 条件4: (stringField LIKE 'test%' OR fixedStringField = 'fixed')
        data1.stringField = "testData123";  // 以'test'开头

        // 条件5: (enumField = 'VALUE_A' OR enumField = 'VALUE_B')
        data1.enumField = TestModule::VALUE_A;  // = 'VALUE_A'

        // 条件6: (boolArray[0] = TRUE OR boolArray[1] = FALSE)
        data1.boolArray[0] = true;   // = TRUE
        data1.boolArray[1] = true;   // 其他值不影响

        datas.push_back(data1);
    }

    // ====== 数据2：通过OR的其他路径匹配 ======
    {
        TestModule::FilterableData data2;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data2.shortField = 0;        // ≠ 1
        data2.ushortField = 2;       // = 2，满足第二个OR条件

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data2.longlongField = 0;     // ≠ 5
        data2.ulonglongField = 6;    // = 6，满足第二个OR条件

        // 条件3: (charField = 'A' OR byteField = 9)
        data2.charField = 'B';       // ≠ 'A'
        data2.byteField = 9;         // = 9，满足第二个OR条件

        // 条件4: (stringField LIKE 'test%' OR fixedStringField = 'fixed')
        data2.stringField = "otherData";  // 不以'test'开头
        data2.fixedStringField = "fixed"; // = 'fixed'，满足第二个OR条件

        // 条件5: (enumField = 'VALUE_A' OR enumField = 'VALUE_B')
        data2.enumField = TestModule::VALUE_B;  // = 'VALUE_B'，满足第二个OR条件

        // 条件6: (boolArray[0] = TRUE OR boolArray[1] = FALSE)
        data2.boolArray[0] = false;  // ≠ TRUE
        data2.boolArray[1] = false;  // = FALSE，满足第二个OR条件

        datas.push_back(data2);
    }

    // ====== 数据3：混合路径匹配 ======
    {
        TestModule::FilterableData data3;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data3.longField = 3;         // = 3，满足第三个OR条件

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data3.floatField = 7.0f;     // = 7.0，满足第三个OR条件

        // 条件3: (charField = 'A' OR byteField = 9)
        data3.charField = 'A';       // = 'A'

        // 条件4: (stringField LIKE 'test%' OR fixedStringField = 'fixed')
        data3.stringField = "testMixed";  // 以'test'开头

        // 条件5: (enumField = 'VALUE_A' OR enumField = 'VALUE_B')
        data3.enumField = TestModule::VALUE_A;  // = 'VALUE_A'

        // 条件6: (boolArray[0] = TRUE OR boolArray[1] = FALSE)
        data3.boolArray[0] = true;   // = TRUE

        datas.push_back(data3);
    }

    // ====== 数据4：所有条件都走最后一个OR选项 ======
    {
        TestModule::FilterableData data4;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data4.ulongField = 4;        // = 4，满足最后一个OR条件

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data4.doubleField = 8.0;     // = 8.0，满足最后一个OR条件

        // 条件3: (charField = 'A' OR byteField = 9)
        data4.byteField = 9;         // = 9，满足最后一个OR条件

        // 条件4: (stringField LIKE 'test%' OR fixedStringField = 'fixed')
        data4.fixedStringField = "fixed"; // = 'fixed'，满足最后一个OR条件

        // 条件5: (enumField = 'VALUE_A' OR enumField = 'VALUE_B')
        data4.enumField = TestModule::VALUE_B;  // = 'VALUE_B'，满足最后一个OR条件

        // 条件6: (boolArray[0] = TRUE OR boolArray[1] = FALSE)
        data4.boolArray[1] = false;  // = FALSE，满足最后一个OR条件

        datas.push_back(data4);
    }

    // ====== 数据5：错误数据 - 条件1的OR都不满足 ======
    {
        TestModule::FilterableData data5;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data5.shortField = 10;       // ≠ 1
        data5.ushortField = 20;      // ≠ 2
        data5.longField = 30;        // ≠ 3
        data5.ulongField = 40;       // ≠ 4

        // 其他条件即使全部满足，整体AND也不满足
        data5.longlongField = 5;     // = 5
        data5.charField = 'A';       // = 'A'
        data5.stringField = "testData";
        data5.enumField = TestModule::VALUE_A;
        data5.boolArray[0] = true;   // = TRUE

        datas.push_back(data5);
    }

    // ====== 数据6：错误数据 - 条件2的OR都不满足 ======
    {
        TestModule::FilterableData data6;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data6.shortField = 1;        // = 1

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data6.longlongField = 10;    // ≠ 5
        data6.ulonglongField = 20;   // ≠ 6
        data6.floatField = 10.0f;    // ≠ 7.0
        data6.doubleField = 20.0;    // ≠ 8.0

        // 其他条件满足也没用
        data6.charField = 'A';
        data6.stringField = "testData";
        data6.enumField = TestModule::VALUE_A;
        data6.boolArray[0] = true;

        datas.push_back(data6);
    }

    // ====== 数据7：错误数据 - 条件3的OR都不满足 ======
    {
        TestModule::FilterableData data7;

        // 条件1: (shortField = 1 OR ushortField = 2 OR longField = 3 OR ulongField = 4)
        data7.shortField = 1;        // = 1

        // 条件2: (longlongField = 5 OR ulonglongField = 6 OR floatField = 7.0 OR doubleField = 8.0)
        data7.longlongField = 5;     // = 5

        // 条件3: (charField = 'A' OR byteField = 9)
        data7.charField = 'B';       // ≠ 'A'
        data7.byteField = 10;        // ≠ 9

        // 其他条件满足也没用
        data7.stringField = "testData";
        data7.enumField = TestModule::VALUE_A;
        data7.boolArray[0] = true;

        datas.push_back(data7);
    }

    // ====== 数据8：错误数据 - 条件4的OR都不满足 ======
    {
        TestModule::FilterableData data8;

        // 条件1-3满足
        data8.shortField = 1;        // = 1
        data8.longlongField = 5;     // = 5
        data8.charField = 'A';       // = 'A'

        // 条件4: (stringField LIKE 'test%' OR fixedStringField = 'fixed')
        data8.stringField = "wrongData";    // 不以'test'开头
        data8.fixedStringField = "wrong";   // ≠ 'fixed'

        // 其他条件满足也没用
        data8.enumField = TestModule::VALUE_A;
        data8.boolArray[0] = true;

        datas.push_back(data8);
    }

    // ====== 数据9：错误数据 - 条件5的OR都不满足 ======
    {
        TestModule::FilterableData data9;

        // 条件1-4满足
        data9.shortField = 1;        // = 1
        data9.longlongField = 5;     // = 5
        data9.charField = 'A';       // = 'A'
        data9.stringField = "testData"; // 以'test'开头

        // 条件5: (enumField = 'VALUE_A' OR enumField = 'VALUE_B')
        data9.enumField = TestModule::VALUE_C;  // 既不是A也不是B

        // 其他条件满足也没用
        data9.boolArray[0] = true;

        datas.push_back(data9);
    }

    // ====== 数据10：错误数据 - 条件6的OR都不满足 ======
    {
        TestModule::FilterableData data10;

        // 条件1-5满足
        data10.shortField = 1;        // = 1
        data10.longlongField = 5;     // = 5
        data10.charField = 'A';       // = 'A'
        data10.stringField = "testData";
        data10.enumField = TestModule::VALUE_A;

        // 条件6: (boolArray[0] = TRUE OR boolArray[1] = FALSE)
        data10.boolArray[0] = false;  // ≠ TRUE
        data10.boolArray[1] = true;   // ≠ FALSE

        datas.push_back(data10);
    }

    StatusCondition& sc = dr->get_statuscondition();
    WaitSet ws;
    ws.attach_condition(sc);
    ConditionSeq active_conditions;
    std::thread th_1(
        [&] {
            for (int i = 0; i < 4; i++) {
                EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

                SequenceData<TestModule::FilterableData> data_seq;
                SampleInfoSeq info_seq;

                EXPECT_EQ(dr->take(
                    data_seq, info_seq, LENGTH_UNLIMITED,
                    ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
                    RETCODE_OK);

                EXPECT_TRUE(data_seq[0].shortField == 1 ||
                    data_seq[0].ushortField == 2 ||
                    data_seq[0].longField == 3 ||
					data_seq[0].ulongField == 4);
                EXPECT_TRUE(data_seq[0].longlongField == 5 ||
                    data_seq[0].ulonglongField == 6 ||
                    data_seq[0].floatField == 7.0f ||
                    data_seq[0].doubleField == 8.0);
                EXPECT_TRUE(data_seq[0].charField == 'A' ||
                    data_seq[0].byteField == 9);
                EXPECT_TRUE(data_seq[0].stringField.find("test") == 0 ||
                    data_seq[0].fixedStringField == "fixed");
                EXPECT_TRUE(data_seq[0].enumField == TestModule::VALUE_A ||
                    data_seq[0].enumField == TestModule::VALUE_B);
                EXPECT_TRUE(data_seq[0].boolArray[0] == true ||
					data_seq[0].boolArray[1] == false);
            }
            EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_TIMEOUT);
        });
    std::thread th_2(
        [&] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            for (const auto& data : datas) {
                EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    th_1.join();
    th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);
 }

 TEST(ContentFilteredTopicTest, ContentFilteredTopic_fuctest4) {
     const char* type_name = TestModule::FilterableDataTypeSupport::get_instance()->get_typename();

     DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
     par_w->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());

     Topic* top_w = par_w->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
     DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
         create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

     DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
     par_r->registe_type(type_name, TestModule::FilterableDataTypeSupport::get_instance());
     Topic* top_r = par_r->create_topic("ContentFilteredTopic", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
     std::string filter_expression = "(shortField >= 10)\
         AND(ushortField <= 20)\
         AND(longField <> 30)\
         AND(doubleField >= 10.5)\
         AND(shortField < longField)\
         AND(floatField >= doubleField)\
         AND('VALUE_B' = enumField)";
     StringSeq expression_parameters;
     ContentFilteredTopic* cftop = par_r->create_contentfilteredtopic("CFT_ContentFilteredTopic", top_r, filter_expression, expression_parameters);
     DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
         create_datareader(cftop, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

     std::vector<TestModule::FilterableData> datas;
     // ====== 数据1：完全匹配所有条件 ======
     {
         TestModule::FilterableData data1;
         data1.shortField = 15; 
         data1.ushortField = 20;
         data1.longField = 100;
         data1.doubleField = 20.0;
         data1.floatField = 25.0f;
         data1.enumField = TestModule::VALUE_B; 

         datas.push_back(data1);
     }

     // ====== 数据2：另一组匹配条件 ======
     {
         TestModule::FilterableData data2;
         data2.shortField = 10;
         data2.ushortField = 0;
         data2.longField = 29;
         data2.doubleField = 10.5;
         data2.floatField = 10.5f;
         data2.enumField = TestModule::VALUE_B;

         datas.push_back(data2);
     }

     // ====== 数据3：错误数据 - shortField不符合 >=10 ======
     {
         TestModule::FilterableData data3;
         data3.shortField = 9;
         data3.ushortField = 20;
         data3.longField = 100;
         data3.doubleField = 20.0;
         data3.floatField = 25.0f;
         data3.enumField = TestModule::VALUE_B;

         datas.push_back(data3);
     }

     // ====== 数据4：错误数据 - ushortField不符合 <=20 ======
     {
         TestModule::FilterableData data4;
         data4.shortField = 15;
         data4.ushortField = 21;

         data4.longField = 100;
         data4.doubleField = 20.0;
         data4.floatField = 25.0f;
         data4.enumField = TestModule::VALUE_B; 

         datas.push_back(data4);
     }

     // ====== 数据5：错误数据 - longField不符合 <>30（等于30） ======
     {
         TestModule::FilterableData data5;
         data5.shortField = 15; 
         data5.ushortField = 20;
         data5.longField = 30; 

         data5.doubleField = 20.0; 
         data5.floatField = 25.0f; 
         data5.enumField = TestModule::VALUE_B; 

         datas.push_back(data5);
     }

     // ====== 数据6：错误数据 - enumField不是VALUE_B ======
     {
         TestModule::FilterableData data6;
         data6.shortField = 15;
         data6.ushortField = 20;
         data6.longField = 100;
         data6.doubleField = 20.0;
         data6.floatField = 25.0f;
         data6.enumField = TestModule::VALUE_A;

         datas.push_back(data6);
     }

     StatusCondition& sc = dr->get_statuscondition();
     WaitSet ws;
     ws.attach_condition(sc);
     ConditionSeq active_conditions;
     std::thread th_1(
         [&] {
             for (int i = 0; i < 2; i++) {
                 EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

                 SequenceData<TestModule::FilterableData> data_seq;
                 SampleInfoSeq info_seq;

                 EXPECT_EQ(dr->take(
                     data_seq, info_seq, LENGTH_UNLIMITED,
                     ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
                     RETCODE_OK);
				 EXPECT_TRUE(data_seq[0].shortField >= 10);
				 EXPECT_TRUE(data_seq[0].ushortField <= 20);
				 EXPECT_TRUE(data_seq[0].longField != 30);
				 EXPECT_TRUE(data_seq[0].doubleField >= 10.5);
				 EXPECT_TRUE(data_seq[0].shortField < data_seq[0].longField);
				 EXPECT_TRUE(data_seq[0].floatField >= data_seq[0].doubleField);
				 EXPECT_EQ(data_seq[0].enumField, TestModule::VALUE_B);

             }
             EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_TIMEOUT);
         });
     std::thread th_2(
         [&] {
             std::this_thread::sleep_for(std::chrono::seconds(1));
             for (const auto& data : datas) {
                 EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
             }
         });
     th_1.join();
     th_2.join();
     par_w->delete_contained_entities();
     par_r->delete_contained_entities();
     TheParticipantFactory->delete_participant(par_w);
     TheParticipantFactory->delete_participant(par_r);
}