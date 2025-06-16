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
#ifndef AGIDDS_COMMON_SEQUENCE_SEQUENCEBASE_H
#define AGIDDS_COMMON_SEQUENCE_SEQUENCEBASE_H

#include <stdint.h>
#include <type_traits>
#include "agidds_dll.h"

class DDS_DLL SequenceBase {
public:
	int32_t maximum() const;

	void* operator [](uint32_t i);

	void** buffer() const;

	virtual int32_t length() const = 0;

	virtual bool length(int32_t newLength) = 0;

	virtual void unloan() = 0;

	virtual ~SequenceBase();

protected:
	SequenceBase() = default;

	int32_t maximum_ = 0;
	int32_t length_ = 0;
	void** elements_ = nullptr;
};

#endif
