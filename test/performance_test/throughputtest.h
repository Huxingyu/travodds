#ifndef THROUGHPUT_TEST_H
#define THROUGHPUT_TEST_H
#include "entity.h"
#include "type.h"

namespace PerformanceTest {

class ThroughputTest : public TestEntity
{
public:
	ThroughputTest(const TestParameter&);

	virtual ~ThroughputTest();

	void start_test();

	virtual void on_data_available(DataReader* reader) override;

	void get_test_result();

	void stop();

private:
	TestData recvData_;

	std::chrono::steady_clock::time_point beginTime_;
	std::chrono::steady_clock::time_point endTime_;
	
	/* 数据包统计 */
	unsigned long long totalCount_ = 0;

	/* 传输数据包序号 */
	unsigned long long lastSn_ = 0;
	bool run_ = true;
};

};
#endif