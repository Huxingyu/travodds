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
#pragma once

#include "SequenceBase.h"
#include <vector>

template<typename T>
class SequenceData : public SequenceBase {
public:
	SequenceData() = default;
	SequenceData(const SequenceData& other);
	SequenceData(int max);

	virtual ~SequenceData();

	T& operator [](unsigned long IDL_i);

	T& operator [](unsigned long IDL_i) const;

	SequenceData& operator =(const SequenceData& other);

	void push_back(T& data);

	virtual int length() const override;

	virtual bool length(int newLength) override;

	virtual void unloan() override;

private:

	void resize(int maximum);

	void release();

	bool from_array(const T array[], unsigned long copyLength);

	bool to_array(T array[], unsigned long copyLength);

	std::vector<T*> datas_;
};

#define AGIDDS_SEQUENCE(FooSeq, Foo) using FooSeq = SequenceData<Foo>;

template<typename T>
SequenceData<T>::SequenceData(const SequenceData& other)
	:SequenceData()
{
	*this = other;
}
template<typename T>
SequenceData<T>::SequenceData(int max)
	: SequenceData()
{
	//resize(max);
}

template<typename T>
SequenceData<T>::~SequenceData()
{

}
template<typename T>
T& SequenceData<T>::operator [](unsigned long element)
{
	if (element >= length_) {
		throw std::out_of_range("out of range");
	}

	return *datas_[element];
}
template<typename T>
T& SequenceData<T>::operator [](unsigned long ele) const
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
	this->maximum_ = other.maximum();

	resize(length_);

	for (int i = 0; i < length_; i++) {
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
	length_++;
	maximum_++;
}
template<typename T>
int SequenceData<T>::length() const
{
	return length_;
}

template<typename T>
bool SequenceData<T>::length(int newLength)
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
void SequenceData<T>::resize(int maximum)
{
	datas_.reserve(maximum);
	datas_.resize(maximum);
	elements_ = reinterpret_cast<void**>(datas_.data());
	while (maximum_ < maximum) {
		T* data = new T();
		datas_[0] = new T();
	}
}

template<typename T>
void SequenceData<T>::release()
{
	for (int i = 0; i < maximum_; ++i) {
		T* data = datas_[i];
		delete data;
	}
	std::vector<T*>().swap(datas_);
}

template<typename T>
bool SequenceData<T>::from_array(const T array[], unsigned long copyLength)
{
	bool ret = length(copyLength);
	if (!ret) {
		return ret;
	}

	for (int i = 0; i < copyLength; i++) {
		datas_[i] = array[i];
	}

	return true;
}

template<typename T>
bool SequenceData<T>::to_array(T array[], unsigned long copyLength)
{
	if (copyLength >= length_) {
		return false;
	}
	for (int i = 0; i < copyLength; i++) {
		datas_[i] = array[i];
	}
	return true;
}