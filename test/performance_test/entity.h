#pragma once
#include "testparameter.h"
#include "dcps/domain/domainparticipantfactory.h"

namespace PerformanceTest {
USING_TRAVODDS_NAMESPACE

static std::string getCurrentTime()
{
	time_t now = time(0); // 获取当前时间的秒数
	struct tm timeInfo;
#ifdef _WIN32
	localtime_s(&timeInfo, &now); // 将时间秒数转换为本地时间结构
#else
	localtime_r(&now, &timeInfo);
#endif

	// 格式化时间为 "时:分:秒" 的格式
	char timeStr[9] = {0}; // 8 个字符用于表示 "时:分:秒"，再加上 null 终止符 '\0'
	std::strftime(timeStr, 9, "%H:%M:%S", &timeInfo);

	return std::string(timeStr);
}

class TestEntity : public DomainParticipantListener {
public:
TestEntity(const TestParameter& param);

virtual ~TestEntity();

void wait_match();

virtual void start_test() = 0;
virtual void stop() =0;

protected:
TestParameter param_;
DataWriter* writer_;
DataReader* reader_;

private:
DomainParticipant* participant_;

};
};