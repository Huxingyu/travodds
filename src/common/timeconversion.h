#pragma once
#include <cstdint>
#include <math.h>
#include "type/basictype.h"
TRAVODDS_NAMESPACE_BEGIN

namespace Conversion {
	/**
	 * @brief 将Time_t格式时间转换为秒并返回double。
	 */
	inline double ToSecondsDouble(const Time_t& t)
	{
		return (double)t.sec + (double)(t.nanosec / pow(10.0, 9));
	}

	/**
	 * @brief 将Duration_t格式时间转换为秒并返回double。
	 */
	inline double ToSecondsDouble(const Duration_t& t) {
		return (double)t.sec + (double)(t.nanosec / pow(10.0, 9));
	}

	/**
	 * @brief 将时间转换为毫秒并返回double。
	 */
	inline double ToMilliSecondsDouble(const Time_t& t) {
		return ((double)t.nanosec / 1000000.0) + (double)t.sec*pow(10.0, 3);
	}

	/**
	 * @brief 将时间转换为毫秒并返回double。
	 */
	inline double ToMilliSecondsDouble(const Duration_t& t) {
		return ((double)t.nanosec / 1000000.0) + (double)t.sec*pow(10.0, 3);
	}

	/**
	 * @brief 将时间转换为微秒并返回double。
	 */
	inline double ToMicroSecondsDouble(const Time_t& t) {
		return ((double)t.nanosec / pow(2.0, 32)*pow(10.0, 6)) + (double)t.sec*pow(10.0, 6);
	}

	/**
	 * @brief 将时间转换为微秒并返回double。
	 */
	inline double ToMicroSecondsDouble(const Duration_t& t) {
		return ((double)t.nanosec / pow(2.0, 32)*pow(10.0, 6)) + (double)t.sec*pow(10.0, 6);
	}


	/**
	 * @brief 将时间转换为毫秒并返回int64。
	 */
	inline int64_t ToMilliSecondsInt64(const Time_t& t) {
		return (int64_t)(t.nanosec / pow(10.0, 6)) + t.sec*(int64_t)pow(10.0, 3);
	}

	/**
	 * @brief 将时间转换为毫秒并返回int64。
	 */
	inline int64_t ToMilliSecondsInt64(const Duration_t& t) {
		return (int64_t)(t.nanosec / pow(10.0, 6)) + t.sec*(int64_t)pow(10.0, 3);
	}

	/**
	 * @brief 将时间转换为微秒返回int64。
	 */
	inline int64_t ToMicroSecondsInt64(const Time_t& t)
	{
		return (int64_t)(t.nanosec / 1000.0) + t.sec*(int64_t)pow(10.0, 6);
	}

	/**
	 * @brief 将时间转换为微秒返回int64。
	 */
	inline int64_t ToMicroSecondsInt64(const Duration_t& t)
	{
		return (int64_t)(t.nanosec / 1000.0) + t.sec*(int64_t)pow(10.0, 6);
	}

	/**
	 * @brief 将double类型秒转换为Time_t。
	 */
	inline Time_t SecondsToTime(double seconds) {
		Time_t t;
		t.sec = ((int32_t)seconds);
		t.nanosec = ((uint32_t)((seconds - (double)t.sec) * pow(10.0, 9)));
		return t;
	}

	/**
	 * @brief 将double类型毫秒转换为Time_t。
	 */
	inline Time_t MilliSecondsToTime(double millisec)
	{
		Time_t t;
		t.sec = ((int32_t)(millisec / pow(10.0, 3)));
		t.nanosec = ((uint32_t)((millisec - (double)t.sec*pow(10.0, 3)) * pow(10.0, 6)));
		return t;
	}

	/**
	 * @brief 将double类型微秒转换为Time_t。
	 */
	inline Time_t MicroSecondsToTime(double microsec)
	{
		Time_t t;
		t.sec = ((int32_t)(microsec / pow(10.0, 6)));
		t.nanosec = ((uint32_t)((microsec - (double)t.sec*pow(10.0, 6)) * pow(10.0, 3)));
		return t;
	}

	/**
	 * @brief 获取两个Time_t之间的绝对差值（以毫秒为单位）并返回double。
	 */
	inline double TimeAbsDiffToDoubleMillisec(const Time_t& t1, const Time_t& t2)
	{
		double result = 0;
		result += (double)abs((t2.sec - t1.sec) * 1000);
		result += (double)std::abs((t2.nanosec - t1.nanosec) / pow(10.0, 6));
		return result;
	}

	/**
	 * @brief 将Duration_t类型转换为Time_t。
	 */
	inline Time_t DurationToTime(Duration_t duration)
	{
		Time_t t;
		t.sec = duration.sec;
		t.nanosec = duration.nanosec;
		return t;
	}

	/**
	 * @brief 将Time_t类型转换为Duration_t。
	 */
	inline Duration_t TimeToDuration(Time_t time)
	{
		Duration_t d;
		d.sec = time.sec;
		d.nanosec = time.nanosec;
		return d;
	}

	/**
	 * @brief 创建一个随机的Time_t，其值为毫秒+[-randoff，randoff]。
	 */
	inline Time_t MilliSecondsWithRandOffsetToTime(double millisec, double randoff)
	{
		randoff = std::abs(randoff);
		millisec = millisec + (-randoff) + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (2 * randoff)));
		return MilliSecondsToTime(millisec);
	}
	/**
	 * @brief 创建一个随机的Time_t，其值为微秒+[-randoff，randoff]
	 */
	inline Time_t MicroSecondsWithRandOffsetToTime(double microsec, double randoff)
	{
		randoff = std::abs(randoff);
		microsec = microsec + (-randoff) + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (2 * randoff)));
		return MicroSecondsToTime(microsec);
	}
	/**
	 * @brief 创建一个随机的Time_t，其值为秒+[-randoff，randoff]
	 */
	inline Time_t SecondsWithRandOffsetToTime(double sec, double randoff)
	{
		randoff = std::abs(randoff);
		sec = sec + (-randoff) + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (2 * randoff)));
		return SecondsToTime(sec);
	}

}
TRAVODDS_NAMESPACE_END

