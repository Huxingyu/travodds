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

#include "Listener/Listener.h"
#include "Status.h"

AGIDDS_NAMESPACE_BEGIN

class DataWriter;

class DataWriterListener : public Listener {
public:
    DataWriterListener() {}

    virtual ~DataWriterListener() {}

    virtual void on_liveliness_lost(DataWriter* the_writer, const LivelinessLostStatus& Status)
    {

    }

    virtual void on_offered_deadline_missed(DataWriter* the_writer, const OfferedDeadlineMissedStatus& Status)
    {

    }

    virtual void on_offered_incompatible_qos(DataWriter* the_writer, const OfferedIncompatibleQosStatus& Status)
    {

    }

    virtual void on_publication_matched(DataWriter* the_writer, const PublicationMatchedStatus& Status)
    {

    }

};

AGIDDS_NAMESPACE_END
