#ifndef DELAYTEST_H
#define DELAYTEST_H

#include <condition_variable>

#include "entity.h"
#include "type.h"

namespace PerformanceTest {

class DelayTest : public TestEntity
{
public:
	DelayTest(const TestParameter&);
	
	virtual ~DelayTest();

	void start_test();

	virtual void on_data_available(DataReader* reader) override;

	void stop();
	
private:
	std::condition_variable_any cva_;
	std::mutex mutex_;

	bool isRecv_ = false;
	TestData recvData_;
	double m_dDelayTestResult = 0;
	/* 用时分布统计 */
	std::vector<int32_t> m_vNumTimes;

	int64_t total_time_us_ = 0;
	std::vector<uint32_t> vElapsedTime_;
	int Num_01_ = 0;	// 0.1ms
	int Num_03_ = 0;	// 0.3ms
	int Num_05_ = 0;	// 0.3ms
	int Num_10_ = 0;	// 1ms
	int Num_15_ = 0;	// 1.5ms
	int Num_20_ = 0;	// 2ms
	int Num_25_ = 0;	// 2.5ms
	int Num_High_ = 0;	// >2.5ms

	bool run_ = true;

};

};
#endif