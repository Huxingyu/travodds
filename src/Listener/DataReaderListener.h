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

class DataReader;
class DataWriter;

class DDS_DLL DataReaderListener : public Listener {
public:

    virtual void on_data_available(DataReader* the_reader)
    {
    }

    virtual void on_sample_rejected(DataReader* the_reader, const SampleRejectedStatus& status)
    {

    }

    virtual void on_liveliness_changed(DataReader* the_reader, const LivelinessChangedStatus& status)
    {

    }

    virtual void on_requested_deadline_missed(DataReader* the_reader, const RequestedDeadlineMissedStatus& status)
    {

    }

    virtual void on_requested_incompatible_qos(DataReader* the_reader, const RequestedIncompatibleQosStatus& status)
    {

    }

    virtual void on_subscription_matched(DataReader* the_reader, const SubscriptionMatchedStatus& status)
    {

    }

    virtual void on_sample_lost(DataReader* the_reader, const SampleLostStatus& status)
    {

    }
};

AGIDDS_NAMESPACE_END
