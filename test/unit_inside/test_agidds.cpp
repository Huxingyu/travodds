#include "gtest/gtest.h"
#include "TestFrame.h"
/************************** YLEND **********************************/
/*    检查指针是否为空     
 *    EXPECT_NE(ptr, nullptr);  
 *    EXPECT_EQ(result, RETCODE_OK);  
 */
void RunSpecificTest(const std::string& testSuiteName, const std::string& testCaseName) {
	std::cout << "Running test suite: " << testSuiteName << ", test case: " << testCaseName << std::endl;
	::testing::GTEST_FLAG(filter) = testSuiteName + "." + testCaseName; // 设置过滤器
	::testing::UnitTest::GetInstance()->Run(); // 运行过滤后的测试用例
}

int32_t main(int argc, char** argv)
{
	{
		const std::string mode = "passive";
		const std::string port_str = "12345";

		std::cout << "Mode: " << mode << std::endl;

		TestFrame::Role role = TestFrame::Role::PASSIVE;
		TestFrame& passive = TestFrame::getInstance(role);
		passive.start(std::stoi(port_str)); // 服务端监听连接

		// Initialize Google Test framework
		//::testing::GTEST_FLAG(output) = "xml:negative.xml";
		::testing::InitGoogleTest(&argc, argv);

		// 主测端通知指定要运行的测试套件和测试用例
		while (true)
		{
			std::string connectionId = "active_connection";
			std::string key = "sync";
			std::string receivedValue;
			TestFrame& frame = TestFrame::getInstance(TestFrame::Role::PASSIVE);
			if (frame.waitForData(connectionId, key, receivedValue, 5000))
			{
				size_t dot_pos = receivedValue.find('.');
				std::string suiteName = receivedValue.substr(0, dot_pos);
				std::string caseName = receivedValue.substr(dot_pos + 1);
				if (suiteName == "test" && caseName == "completed")
				{
					std::cout << "test completed" << std::endl;
					break;
				}
				RunSpecificTest(suiteName, caseName);
			}
		}

		passive.stop();
		passive.setRole(TestFrame::Role::ACTIVE);
	}





	{
		const std::string mode = "active";
		const std::string port_str = "54321";
		const std::string remote_ip = "127.0.0.1";
		std::cout << "Mode: " << mode << std::endl;
		std::cout << "Mode: " << mode << std::endl;
		std::cout << "Mode: " << mode << std::endl;
		std::cout << "Mode: " << mode << std::endl;
		std::cout << "Mode: " << mode << std::endl;
		std::cout << "Mode: " << mode << std::endl;

		TestFrame::Role role = TestFrame::Role::ACTIVE;
		TestFrame& active = TestFrame::getInstance(role);
		active.start(remote_ip, std::stoi(port_str)); // 客户端连接到服务端

		// Initialize Google Test framework
		//testing::GTEST_FLAG(output) = "xml:positive.xml";
		testing::InitGoogleTest(&argc, argv);

		::testing::GTEST_FLAG(filter) = "*Positive";
		// 根据参数运行测试用例
		int retCode = RUN_ALL_TESTS();
		EXPECT_TRUE(active.sendData("active_connection", "sync", "test.completed"));

		active.stop();
	}



	::testing::GTEST_FLAG(filter) = "-*Positive*:*Negative*";
	RUN_ALL_TESTS();
    return 0;
}