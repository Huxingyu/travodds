#include "gtest/gtest.h"

/************************** YLEND **********************************/
/*    检查指针是否为空     
 *    EXPECT_NE(ptr, nullptr);  
 *    EXPECT_EQ(result, RETCODE_OK);  
 */
int32_t main(int argc, char** argv)
{
    // 初始化 Google Test
    testing::InitGoogleTest(&argc, argv);

    // testing::GTEST_FLAG(filter) = "QoSTest.QoS_ResourceLimitsQosPolicy";

    // 运行所有测试
    RUN_ALL_TESTS();

    return 0;
}