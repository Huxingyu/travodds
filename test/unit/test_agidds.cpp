//#define TEST_DOMAINPARTICIPANTFACTORY
//#define TEST_DOMAINPARTICIPANT
//#define TEST_TOPICDESCRIPTION
//#define TEST_TOPIC
//#define TEST_CONTENtFILTEREDTOPIC
//#define TEST_MULTITOPIC
//#define TEST_PUBLISHER
//#define TEST_DATAWRITER
//#define TEST_SUBSCRIBER
//#define TEST_DATAREADER
//#define TEST_READCONDITION
//#define TEST_QUERYCONDITION
//#define TEST_MULTI_CREATE_DELETE
#define TEST_ALL
#ifdef TEST_ALL
#define TEST_DOMAINPARTICIPANTFACTORY
#define TEST_DOMAINPARTICIPANT
#define TEST_TOPICDESCRIPTION
#define TEST_TOPIC
#define TEST_CONTENtFILTEREDTOPIC
#define TEST_MULTITOPIC
#define TEST_PUBLISHER
#define TEST_DATAWRITER
#define TEST_SUBSCRIBER
#define TEST_DATAREADER
#define TEST_READCONDITION
#define TEST_QUERYCONDITION
#endif // TEST_ALL

#include "test_head.hpp"

#include "test_domainparticipantfactory.cpp"
#include "test_domainparticipant.cpp"
#include "test_topicdescription.cpp"
#include "test_topic.cpp"
#include "test_contentfilteredtopic.cpp"
#include "test_multitopic.cpp"
#include "test_publisher.cpp"
#include "test_datawriter.cpp"
#include "test_subscriber.cpp"
#include "test_datareader.cpp"
#include "test_readcondition.cpp"
#include "test_querycondition.cpp"
#include "test_multi_create_delete.cpp"


/************************** YLEND **********************************/
/*    检查指针是否为空     
 *    EXPECT_NE(ptr, nullptr);  
 *    EXPECT_EQ(result, RETCODE_OK);  
 */
int32_t main(int argc, char** argv)
{
    // 初始化 Google Test
    testing::InitGoogleTest(&argc, argv);

    // 注册自定义测试环境
    testing::AddGlobalTestEnvironment(new DDSTest);

    // 运行所有测试
    RUN_ALL_TESTS();

    return 0;
}