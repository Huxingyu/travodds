#ifndef QOSCOMPATIBILITYTESTNEGATIVE_H
#define QOSCOMPATIBILITYTESTNEGATIVE_H

#include "gtest/gtest.h"
#include "dcps/listener/datareaderlistener.h"

USING_TRAVODDS_NAMESPACE

// BaseDataReaderListener
class BaseDataReaderListener : public DataReaderListener {
public:
    BaseDataReaderListener()
        : requested_incompatible_qos_flag(false), requested_deadline_missed_flag(false),
        liveliness_changed_flag(false), subscription_matched_flag(false),
        sample_lost_flag(false), sample_rejected_flag(false), data_available_flag(false) {}

    void on_requested_incompatible_qos(DataReader*, const RequestedIncompatibleQosStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx1);
            requested_incompatible_qos_flag = true;
        }
        cv1.notify_one();
    }
    int wait_requested_incompatible_qos(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx1);
        if (cv1.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return requested_incompatible_qos_flag; }))
            return 0;
        return 1;
    }

    void on_requested_deadline_missed(DataReader*, const RequestedDeadlineMissedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx2);
            requested_deadline_missed_flag = true;
        }
        cv2.notify_one();
    }
    int wait_requested_deadline_missed(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx2);
        if (cv2.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return requested_deadline_missed_flag; }))
            return 0;
        return 1;
    }

    void on_liveliness_changed(DataReader*, const LivelinessChangedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx3);
            liveliness_changed_flag = true;
        }
        cv3.notify_one();
    }
    int wait_liveliness_changed(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx3);
        if (cv3.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return liveliness_changed_flag; }))
            return 0;
        return 1;
    }

    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx4);
            subscription_matched_flag = true;
        }
        cv4.notify_one();
    }
    int wait_subscription_matched(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx4);
        if (cv4.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return subscription_matched_flag; }))
            return 0;
        return 1;
    }

    void on_sample_lost(DataReader*, const SampleLostStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx5);
            sample_lost_flag = true;
        }
        cv5.notify_one();
    }
    int wait_sample_lost(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx5);
        if (cv5.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return sample_lost_flag; }))
            return 0;
        return 1;
    }

    void on_sample_rejected(DataReader*, const SampleRejectedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx6);
            sample_rejected_flag = true;
        }
        cv6.notify_one();
    }
    int wait_sample_rejected(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx6);
        if (cv6.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return sample_rejected_flag; }))
            return 0;
        return 1;
    }

    void on_data_available(DataReader*) override {
        {
            std::lock_guard<std::mutex> lock(mtx7);
            data_available_flag = true;
        }
        cv7.notify_one();
    }
    int wait_data_available(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx7);
        if (cv7.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return data_available_flag; }))
            return 0;
        return 1;
    }

private:
    std::mutex mtx1, mtx2, mtx3, mtx4, mtx5, mtx6, mtx7;
    std::condition_variable cv1, cv2, cv3, cv4, cv5, cv6, cv7;
    bool requested_incompatible_qos_flag;
    bool requested_deadline_missed_flag;
    bool liveliness_changed_flag;
    bool subscription_matched_flag;
    bool sample_lost_flag;
    bool sample_rejected_flag;
    bool data_available_flag;
};

class DurabilityMatchDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

class ReliabilityMatchDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

class DeadlineMatchDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

class OwnershipMatchDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

class LivelinessMatchDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};


class QoSCompatibilityTestNegative : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
};

#endif // QOSCOMPATIBILITYTESTNEGATIVE_H
