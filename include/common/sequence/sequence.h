//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef TRAVODDS_COMMON_SEQUENCE_SEQUENCE_H
#define TRAVODDS_COMMON_SEQUENCE_SEQUENCE_H

#include "common/sequence/sequencebase.h"
#include <vector>

/**
 * @brief SequenceData模板类,Sequence序列具体功能实现类。
 */
template<typename T>
class SequenceData : public SequenceBase {
public:
	/**
	 * @brief 默认构造函数。
	 */
	SequenceData() = default;
	/**
	 * @brief 构造函数。
	 * 
	 * @param [in] other SequenceData序列。
	 */
	SequenceData(const SequenceData& other);
	/**
	 * @brief 构造函数。
	 * 
	 * @param [in] max 序列空间大小最大值。
	 */
	SequenceData(int32_t max);
	/**
	 * @brief 析构函数。
	 */
	virtual ~SequenceData();
	/**
	 * @brief 获取序列下标对应的元素。
	 *
	 * @param [in] IDL_i 序列元素下标。
	 *
	 * @return 下标对应的序列元素。
	 */
	T& operator [](uint32_t IDL_i);
	/**
	 * @brief 获取序列下标对应的元素。
	 *
	 * @param [in] IDL_i 序列元素下标。
	 *
	 * @return 下标对应的序列元素。
	 */
	T& operator [](uint32_t IDL_i) const;

	/**
	 * @brief 序列类深拷贝函数。
	 */
	SequenceData& operator =(const SequenceData& other);

	/**
	 * @brief 序列元素存储函数。
	 *
	 * @param [in] data 要加入序列的元素。
	 */
	void push_back(T& data);
	/**
	 * @brief 获取序列当前长度。
	 *
	 * @return 序列当前长度。
	 */
	virtual int32_t length() const override;

	/**
	 * @brief 设置序列最大长度。
	 *
	 * @param [in] newLength 序列新的最大长度。
	 */
	virtual bool length(int32_t newLength) override;
	/**
	 * @brief 序列空间释放。
	 */
	virtual void unloan() override;

private:

	/**
	 * @brief 设置序列空间及最大长度。
	 *
	 * @param [in] maximum 新的序列空间最大长度。
	 */
	void resize(uint32_t maximum);

	/**
	 * @brief 序列空间释放。
	 */
	void release();

	/**
	 * @brief 从序列元素数组中拷贝元素到序列中。
	 *
	 * @param [in] array	  序列元素数组。
	 * @param [in] copyLength 序列元素数组长度。
	 * 
	 * @return 拷贝成功返回true，否则返回false。
	 */
	bool from_array(const T array[], uint32_t copyLength);

	/**
	 * @brief 从序列中拷贝元素到与序列元素类型相同的数组中。
	 *
	 * @param [out] array	   序列元素数组。
	 * @param [in]  copyLength 序列元素数组长度。
	 *
	 * @return 拷贝成功返回true，否则返回false。
	 */
	bool to_array(T array[], uint32_t copyLength);

	std::vector<T*> datas_;
};

#define TRAVODDS_SEQUENCE(FooSeq, Foo) using FooSeq = SequenceData<Foo>;

template<typename T>
SequenceData<T>::SequenceData(const SequenceData& other)
	:SequenceData()
{
	*this = other;
}
template<typename T>
SequenceData<T>::SequenceData(int32_t max)
	: SequenceData()
{
	resize(max);
	this->length_ = max;
}

template<typename T>
SequenceData<T>::~SequenceData()
{
	release();
}
template<typename T>
T& SequenceData<T>::operator [](uint32_t element)
{
	if (element >= length_) {
		throw std::out_of_range("out of range");
	}

	return *datas_[element];
}
template<typename T>
T& SequenceData<T>::operator [](uint32_t ele) const
{
	if (ele >= length_) {
		throw std::out_of_range("out of range");
	}

	return *datas_[ele];
}

template<typename T>
SequenceData<T>& SequenceData<T>::operator =(const SequenceData& other)
{
	this->length_ = other.length();
	resize(length_);

	for (int32_t i = 0; i < length_; i++) {
		//TODO
		*datas_[i] = other[i];
	}
	return *this;
}
template<typename T>
void SequenceData<T>::push_back(T& data)
{
	T* value = new T();
	*value = data;

	datas_.push_back(value);
	elements_ = reinterpret_cast<void**>(datas_.data());
	length_++;
	maximum_++;
}
template<typename T>
int32_t SequenceData<T>::length() const
{
	return length_;
}

template<typename T>
bool SequenceData<T>::length(int32_t newLength)
{
	if (newLength < 0) {
		return false;
	}
	if (newLength <= maximum_) {
		length_ = newLength;
		return true;
	}

	resize(newLength);
	length_ = newLength;

	return true;

}
template<typename T>
void SequenceData<T>::unloan()
{
	release();
}
template<typename T>
void SequenceData<T>::resize(uint32_t maximum) {
	datas_.reserve(maximum);
	datas_.resize(maximum);
	elements_ = reinterpret_cast<void**>(datas_.data());
	while (maximum_ < maximum) {
		datas_[maximum_++] = new T();
	}
}

template<typename T>
void SequenceData<T>::release()
{
	for (int32_t i = 0; i < maximum_; ++i) {
		T* data = datas_[i];
		delete data;
	}
	std::vector<T*>().swap(datas_);

	maximum_ = 0;
    length_ = 0;
	elements_ = nullptr;
}

template<typename T>
bool SequenceData<T>::from_array(const T array[], uint32_t copyLength)
{
	bool ret = length(copyLength);
	if (!ret) {
		return ret;
	}

	for (int32_t i = 0; i < copyLength; i++) {
		*datas_[i] = array[i];
	}

	return true;
}

template<typename T>
bool SequenceData<T>::to_array(T array[], uint32_t copyLength)
{
	if (copyLength >= length_) {
		return false;
	}
	for (int32_t i = 0; i < copyLength; i++) {
		array[i] = *datas_[i];
	}
	return true;
}

#endif
