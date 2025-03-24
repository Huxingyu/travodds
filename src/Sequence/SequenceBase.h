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
#include <stdint.h>
#include <type_traits>
#include "AGIDDS_DLL.h"

class DDS_DLL SequenceBase {
public:
	int maximum() const;

	void* operator [](unsigned long i);

	void** buffer() const;

	virtual int length() const = 0;

	virtual bool length(int newLength) = 0;

	virtual void unloan() = 0;

	virtual ~SequenceBase();

protected:
	SequenceBase() = default;

	int maximum_ = 0;
	int length_ = 0;
	void** elements_ = nullptr;
};

