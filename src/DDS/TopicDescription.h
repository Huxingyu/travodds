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

#include "AGIDDS_DLL.h"

AGIDDS_NAMESPACE_BEGIN

class DomainParticipant;
class DDS_DLL TopicDescription {
public:
    virtual ~TopicDescription() = default;

    virtual DomainParticipant* get_participant() const = 0;

    virtual const char* get_type_name() const = 0;

    virtual const char* get_name() const = 0;

};

AGIDDS_NAMESPACE_END
