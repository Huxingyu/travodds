#include <algorithm>
#include "delaytest.h"

using namespace PerformanceTest;

DelayTest::DelayTest(const TestParameter& param): 
    TestEntity(param)
{
	fprintf(stderr, "The sending end and the receiving end are successfully connected, and the delay test starts . . \n");
}

DelayTest::~DelayTest()
{
}

void DelayTest::start_test()
{
    wait_match();

	TestData instance;
	InstanceHandle_t instance_handle = HANDLE_NIL;
	ReturnCode_t retcode;

	instance.data.resize(param_.dataSize -16,'a');
	instance.size = param_.dataSize;
	instance.sn = 0;

	if (param_.entityType == EntityType::PUB)
	{
		fprintf(stderr, "Delay test: sending\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
		int count = 0;	
		std::chrono::steady_clock::time_point last_enter_time, current_enter_time;
		double dVariance = 0.0;
		int Num_01 = 0;	// 0.1ms
		int Num_03 = 0;	// 0.3ms
		int Num_05 = 0;	// 0.3ms
		int Num_10 = 0;	// 1ms
		int Num_15 = 0;	// 1.5ms
		int Num_20 = 0;	// 2ms
		int Num_25 = 0;	// 2.5ms
		int Num_High = 0;	// >2.5ms
		int64_t total_time_us = 0;
		std::vector<uint32_t> vElapsedTime;
		volatile bool flag = true;
		while (flag && run_)
		{
			current_enter_time = std::chrono::steady_clock::now();
			count++;
			if (count > 1) {
				auto elapsed_time_us = std::chrono::duration_cast<std::chrono::microseconds>(current_enter_time - last_enter_time).count();
				total_time_us += elapsed_time_us;
				vElapsedTime.push_back(elapsed_time_us);
				if (elapsed_time_us < 200)//0.1
					Num_01++;
				else if (elapsed_time_us < 600)//0.3
					Num_03++;
				else if (elapsed_time_us < 1000)//0.5
					Num_05++;
				else if (elapsed_time_us < 2000)//1
					Num_10++;
				else if (elapsed_time_us < 3000)//1.5
					Num_15++;
				else if (elapsed_time_us < 4000)//2
					Num_20++;
				else if (elapsed_time_us < 5000)//2.5
					Num_25++;
				else
					Num_High++;
			}

            flag = count <= param_.counts && total_time_us / 1e6 <= param_.times;
            if (flag == false) {
				break;
            }
			last_enter_time = std::chrono::steady_clock::now();
			retcode = writer_->write(&instance, instance_handle);
			if (retcode != RETCODE_OK) {
				continue;
				fprintf(stderr, "write error %d\n", retcode);
			}

			std::unique_lock<std::mutex> lock(mutex_);

			bool status = cva_.wait_for(lock, std::chrono::seconds(10), [this](){return  !run_ || isRecv_;});
			if (status = false) {
				break;
			}
			isRecv_ = false;
		}

		this->m_dDelayTestResult = (double)total_time_us /( (double)(count-1) *2);
		this->m_vNumTimes.clear();
		this->m_vNumTimes.push_back(Num_01);
		this->m_vNumTimes.push_back(Num_03);
		this->m_vNumTimes.push_back(Num_05);
		this->m_vNumTimes.push_back(Num_10);
		this->m_vNumTimes.push_back(Num_15);
		this->m_vNumTimes.push_back(Num_20);
		this->m_vNumTimes.push_back(Num_25);
		this->m_vNumTimes.push_back(Num_High);
		int32_t percent99 = 0;
		int32_t mind = 0;
		int32_t maxd = 0;
		if (!vElapsedTime.empty()) {
			std::sort(vElapsedTime.begin(), vElapsedTime.end());
			percent99 = vElapsedTime[vElapsedTime.size() * 0.99] / 2;
			mind = vElapsedTime[0] / 2;
			maxd = vElapsedTime[vElapsedTime.size() - 1] / 2;
		}

		char tReslut[1024] = { 0 };
		//                Ê±´Á              0.1 0.3 0.5  1   1.5  2 2.5  <   99%  min max   
		sprintf(tReslut, "%-9d%-7d | %-7d%-7d%-7d%-7d%-7d%-7d%-7d%-7d%-8d%-8d%-8d| %f",
			param_.dataSize, count - 1,
			m_vNumTimes[0], m_vNumTimes[1], m_vNumTimes[2], m_vNumTimes[3], m_vNumTimes[4], m_vNumTimes[5], m_vNumTimes[6], m_vNumTimes[7],
			percent99, mind, maxd, m_dDelayTestResult);
		std::cout << "\n----------------------------------------------------------------------------------------------------------------------" << std::endl;
		std::cout << "                 | Distribution of time                                                            |" << std::endl;
		std::cout << "Size(B)  Count   | 0.1ms  0.3ms  0.5ms  1.0ms  1.5ms  2.0ms  2.5ms  >2.5ms 99%(us) Min(us) Max(us) | Average delay(us)" << std::endl;
		std::cout << tReslut << std::endl;
		std::cout << "----------------------------------------------------------------------------------------------------------------------\n" << std::endl;
	}
	else
	{
	    fprintf(stderr, "Delay test: receiving\n");
		while (run_) {
			std::unique_lock<std::mutex> lock(mutex_);

			bool status = cva_.wait_for(lock, std::chrono::seconds(15), [this]() {return  !run_ || isRecv_; });
			if (status == false|| !run_) {
				run_ = false;
				break;
			}
			isRecv_ = false;
			retcode = writer_->write(&recvData_, instance_handle);
			if (retcode != RETCODE_OK) {
				fprintf(stderr, "write error %d\n", retcode);
			}
		}
	}
	return;
}

void DelayTest::on_data_available(DataReader* reader)
{

	SampleInfo info;
	ReturnCode_t retcode;
	retcode = reader->take_next_sample(&recvData_, info);

	if (retcode == RETCODE_NO_DATA) {
		return;
	}
	else if (retcode != RETCODE_OK) {
		fprintf(stderr, "take error %d\n", retcode);
		return;
	}
	isRecv_ = true;
	cva_.notify_one();
}

void PerformanceTest::DelayTest::stop()
{ 
    run_ = false;
	isRecv_ = true;
	cva_.notify_one();
}




