#ifndef QOSCOMPATIBILITYTESTPOSITIVE_H
#define QOSCOMPATIBILITYTESTPOSITIVE_H

#include "gtest/gtest.h"
#include "dcps/listener/datawriterlistener.h"

USING_TRAVODDS_NAMESPACE

// BaseDataWriterListener
class BaseDataWriterListener : public DataWriterListener {
public:
    BaseDataWriterListener()
        : liveliness_lost_flag(false), offered_deadline_missed_flag(false),
        offered_incompatible_qos_flag(false), publication_matched_flag(false) {}

    void on_liveliness_lost(DataWriter*, const LivelinessLostStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx1);
            liveliness_lost_flag = true;
        }
        cv1.notify_one();
    }
    int wait_liveliness_lost(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx1);
        if (cv1.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return liveliness_lost_flag; }))
            return 0;
        return 1;
    }

    void on_offered_deadline_missed(DataWriter*, const OfferedDeadlineMissedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx2);
            offered_deadline_missed_flag = true;
        }
        cv2.notify_one();
    }
    int wait_offered_deadline_missed(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx2);
        if (cv2.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return offered_deadline_missed_flag; }))
            return 0;
        return 1;
    }

    void on_offered_incompatible_qos(DataWriter*, const OfferedIncompatibleQosStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx3);
            offered_incompatible_qos_flag = true;
        }
        cv3.notify_one();
    }
    int wait_offered_incompatible_qos(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx3);
        if (cv3.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return offered_incompatible_qos_flag; }))
            return 0;
        return 1;
    }

    void on_publication_matched(DataWriter*, const PublicationMatchedStatus&) override {
        {
            std::lock_guard<std::mutex> lock(mtx4);
            publication_matched_flag = true;
        }
        cv4.notify_one();
    }
    int wait_publication_matched(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx4);
        if (cv4.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return publication_matched_flag; }))
            return 0;
        return 1;
    }

private:
    std::mutex mtx1, mtx2, mtx3, mtx4;
    std::condition_variable cv1, cv2, cv3, cv4;
    bool liveliness_lost_flag;
    bool offered_deadline_missed_flag;
    bool offered_incompatible_qos_flag;
    bool publication_matched_flag;
};

class DurabilityMatchDataWriterListener : public BaseDataWriterListener {
    // ֱ�Ӽ̳�BaseDataWriterListener���������ʵ��
};

class ReliabilityMatchDataWriterListener : public BaseDataWriterListener {
    // ֱ�Ӽ̳�BaseDataWriterListener���������ʵ��
};

class DeadlineMatchDataWriterListener : public BaseDataWriterListener {
    // ֱ�Ӽ̳�BaseDataWriterListener���������ʵ��
};

class OwnershipMatchDataWriterListener : public BaseDataWriterListener {
    // ֱ�Ӽ̳�BaseDataWriterListener���������ʵ��
};

class LivelinessMatchDataWriterListener : public BaseDataWriterListener {
    // ֱ�Ӽ̳�BaseDataWriterListener���������ʵ��
};

class QoSCompatibilityTestPositive : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
};

#endif // QOSCOMPATIBILITYTESTPOSITIVE_H
