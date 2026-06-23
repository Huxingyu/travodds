#include "throughputtest.h"
#include "type.h"
using namespace PerformanceTest;


ThroughputTest::ThroughputTest(const TestParameter& param):
	TestEntity(param)
{
	fprintf(stderr, "The sending end and the receiving end are successfully connected, and the throughput test starts . . \n");
}

ThroughputTest::~ThroughputTest()
{
}


void ThroughputTest::start_test()
{
	wait_match();

	if (param_.entityType == EntityType::SUB)
	{
		std::cout << "Throughput test: receiving start" << std::endl;
		while (run_) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	} else {
		std::cout << "Throughput test: sending start" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
		TestData instance;
		InstanceHandle_t instance_handle = HANDLE_NIL;
		ReturnCode_t retcode;

		instance.data.resize(param_.dataSize - 16u, 'a');
		instance.size = param_.dataSize;
		instance.sn = 0;

		int64_t total_time = 0;
		beginTime_ = std::chrono::steady_clock::now();
		while (++totalCount_ <= param_.counts && total_time <= param_.times && run_)
		{
			instance.sn = totalCount_;
			retcode = writer_->write(&instance, instance_handle);
			if (retcode != RETCODE_OK)
			{
				fprintf(stderr, "write error %d\n", retcode);
				totalCount_--;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			total_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - beginTime_).count();
		}
	    totalCount_--;
		endTime_ = std::chrono::steady_clock::now();
	}
	get_test_result();
	
}


void ThroughputTest::on_data_available(DataReader* reader)
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
	if (recvData_.sn == 1) {
		beginTime_ = std::chrono::steady_clock::now();
		totalCount_ = 1;
		lastSn_ = 1;
		param_.dataSize = recvData_.size;
	} else  {
		totalCount_++;
		lastSn_ = recvData_.sn;
	}

	endTime_ = std::chrono::steady_clock::now();
	//ThroughPutDataType_reader->return_loan(data_seq, info_seq);
}


void ThroughputTest::get_test_result()
{
	auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(endTime_ - beginTime_).count();
	double average_throught = totalCount_ * param_.dataSize * 8 / (double(elapsed_time * 1e6));

	std::string result_str = "";
	std::cout << "\n-----------------------------------------------------------------------------------------------------------" << std::endl;

	if (param_.entityType == EntityType::PUB)
	{
		std::cout << "Data size    Maximum send    Pub average throughput(Mbps)" << std::endl;
		char tReslut[1024] = { 0 };
		std::string cTime = getCurrentTime();
		sprintf(tReslut, "%-13s%-16s%-31s",std::to_string(param_.dataSize).c_str(), std::to_string(totalCount_).c_str(), std::to_string(average_throught).c_str());
		std::cout << tReslut << std::endl;
	}
	else {
		std::cout << "Data size    Maximum recv    Sub average throughput(Mbps)   Average loss rate" << std::endl;
		double lossRate = lastSn_ == 0 ? 0: double(lastSn_ - totalCount_) / lastSn_;
		char tReslut[1024] = { 0 };
		sprintf(tReslut, "%-13s%-16s%-31s%.2f %%", std::to_string(param_.dataSize).c_str(), std::to_string(totalCount_).c_str(), std::to_string(average_throught).c_str(), lossRate *100);
		std::cout << tReslut <<std::endl;
	}
}

void PerformanceTest::ThroughputTest::stop()
{
   run_ = false;
}

