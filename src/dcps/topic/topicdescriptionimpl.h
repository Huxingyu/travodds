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
#ifndef AGIDDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H
#define AGIDDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H

#include "dcps/topic/topicdescription.h"
#include <string>
#include <atomic>
AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
class TopicDescriptionImpl : public virtual TopicDescription {
public:
    TopicDescriptionImpl(const std::string& name, const std::string& type_name, DomainParticipantImpl* participant);

    virtual ~TopicDescriptionImpl();

    DomainParticipant* get_participant() const;

    const char* get_type_name() const;

    const char* get_name() const;

public:
    /* 引用 */
    void Reference();
    /* 释放引用 */
    void Dereference();
    /* 是否被引用 */
    bool IsReferenced();
protected:
    std::string name_;
    std::string typeName_;
    DomainParticipantImpl* participant_ = nullptr;

    /* 引用计数 */
    std::atomic_size_t refs_;
};
AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H
