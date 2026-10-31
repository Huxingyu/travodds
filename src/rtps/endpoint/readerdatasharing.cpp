// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "rtps/endpoint/reader.h"
#include "rtps/discovery/discoverydata.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "common/memory/sharedpayloadpool.h"
#include "common/log/logger.h"
#include <thread>
#include <limits>

USING_TRAVODDS_NAMESPACE;

// Matching only schedules work. Mapping and registration never hold a DDS table,
// ingress, or connection-map lock. Cancellation wakes bounded retry waits.
struct Reader::NativeConnection {
#if defined(__linux__)
    SharedPoolSession session;
    std::atomic<bool> stopping{false};
    std::mutex threadMutex, waitMutex;
    std::condition_variable wake;
    std::thread thread;
    void cancel() { stopping.store(true); wake.notify_all(); }
    void detach() {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (thread.joinable()) thread.detach();
    }
    void stop() {
        cancel();
        std::thread pending;
        {
            std::lock_guard<std::mutex> lock(threadMutex);
            if (thread.joinable()) {
                if (thread.get_id() == std::this_thread::get_id()) thread.detach();
                else pending = std::move(thread);
            }
        }
        if (pending.joinable()) pending.join();
    }
    ~NativeConnection() { stop(); }
#endif
};

bool Reader::DataSharingSessionChanged(const DiscoveredWriterData& writer) {
#if defined(__linux__)
    std::lock_guard<std::mutex> lock(nativeConnectionsMutex_);
    auto found = nativeConnections_.find(writer.remoteWriterGuid);
    if (found == nativeConnections_.end()) return false;
    const auto& info = writer.ddsPublicationData.data_sharing;
    return !(found->second->session == SharedPoolSession{info.session_high, info.session_low});
#else
    (void)writer; return false;
#endif
}

bool Reader::StartDataSharing(const std::shared_ptr<Reader>& self, const DiscoveredWriterData& writer) {
#if defined(__linux__)
    if (guid_.entityId.entityKind != ENTITYKIND_USER_READER_NO_KEY &&
        guid_.entityId.entityKind != ENTITYKIND_USER_READER_WITH_KEY) return false;
    SharedPoolDescriptor descriptor;
    NativeTypeInfo type;
    bool replay = false;
    {
        std::lock_guard<std::recursive_mutex> ingress(dataMutex_);
        if (!datareader_ || self.get() != this) return false;
        const auto& info = writer.ddsPublicationData.data_sharing;
        if (!info.compatible_with(datareader_->GetBuiltinTopicData().data_sharing) ||
            info.segment_bytes > qos_.data_sharing.max_segment_bytes) return false;
        type = info.type;
        descriptor.writer = writer.remoteWriterGuid;
        descriptor.session = {info.session_high, info.session_low};
        descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session,
            qos_.data_sharing.shared_memory_directory);
        descriptor.config.payload_size = info.type.size;
        descriptor.config.alignment = std::max(uint32_t(64), info.type.alignment);
        descriptor.config.slots = info.slots;
        descriptor.config.max_readers = info.max_readers;
        descriptor.config.max_segment_bytes = qos_.data_sharing.max_segment_bytes;
        descriptor.segment_bytes = info.segment_bytes;
        replay = qos_.durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS &&
            writer.ddsPublicationData.durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS;
    }
    try {
        std::lock_guard<std::mutex> lock(nativeConnectionsMutex_);
        if (nativeStopping_ || !MatchedWriterIsMatched(descriptor.writer)) return false;
        auto old = nativeConnections_.find(descriptor.writer);
        if (old != nativeConnections_.end()) {
            if (old->second->session == descriptor.session) return true;
            // Matching may hold Participant locks. Never join an old worker here.
            old->second->cancel(); old->second->detach();
            nativeConnections_.erase(old);
        }
        auto runtime = std::make_shared<NativeConnection>();
        runtime->session = descriptor.session;
        nativeConnections_.emplace(descriptor.writer, runtime);
        try {
            runtime->thread = std::thread([runtime, weak = std::weak_ptr<Reader>(self),
                    descriptor, type, replay, readerGuid = guid_] {
                std::shared_ptr<SharedPoolReader> registration;
                std::shared_ptr<SharedPayloadPool> pool;
                unsigned attempts = 0;
                auto delay = std::chrono::milliseconds(25);
                while (!runtime->stopping.load() && !registration) {
                    if (weak.expired()) return;
                    bool terminal = false;
                    try {
                        pool = SharedPayloadPool::open(descriptor);
                        SharedPoolStats stats;
                        auto result = pool->statistics(stats);
                        if (result == SharedPoolResult::CORRUPT || result == SharedPoolResult::CLOSED ||
                            (result == SharedPoolResult::OK && !stats.writer_alive)) terminal = true;
                        else if (result == SharedPoolResult::OK) registration = pool->registerReader(readerGuid, replay);
                    } catch (const std::invalid_argument& error) {
                        TRAVODDS_LOG(LOG_WARNING, "DataSharing non-retryable session/layout: %s", error.what());
                        return;
                    } catch (const std::exception& error) {
                        if (attempts == 0) TRAVODDS_LOG(LOG_WARNING,
                            "DataSharing fallback to RTPS; scheduling mapping retry: %s", error.what());
                    }
                    if (terminal) {
                        TRAVODDS_LOG(LOG_WARNING, "DataSharing non-retryable closed/corrupt session; awaiting endpoint recreation");
                        return;
                    }
                    if (registration) break;
                    if (attempts == 0) TRAVODDS_LOG(LOG_INFO, "DataSharing retry pending (mapping or registration capacity)");
                    ++attempts;
                    std::unique_lock<std::mutex> wait(runtime->waitMutex);
                    runtime->wake.wait_for(wait, delay, [&] { return runtime->stopping.load(); });
                    delay = std::min(delay * 2, std::chrono::milliseconds(1000));
                }
                if (!registration) return;
                if (runtime->stopping.load()) { registration->close(); return; }
                TRAVODDS_LOG(LOG_INFO, "DataSharing mapping/registration successful after %u retries", attempts);
                uint64_t last = 0;
                try {
                    while (!runtime->stopping.load()) {
                        SharedReadLoan loan;
                        const auto result = registration->waitNext(last, std::chrono::milliseconds(100), loan);
                        if (result == SharedPoolResult::TIMEOUT) {
                            // Liveness probes are control-plane work, outside
                            // the no-mutex wait/take data plane. A busy metadata
                            // manager returns TIMEOUT rather than blocking stop.
                            SharedPoolStats stats;
                            const auto state = pool->statistics(stats);
                            if (state == SharedPoolResult::CORRUPT || state == SharedPoolResult::CLOSED ||
                                (state == SharedPoolResult::OK && !stats.writer_alive)) break;
                            continue;
                        }
                        if (result != SharedPoolResult::OK) break;
                        last = loan.sequence();
                        auto reader = weak.lock();
                        if (!reader || runtime->stopping.load()) break;
                        reader->ReceiveNativeSample(descriptor.writer, type, std::move(loan));
                    }
                } catch (const std::exception& error) {
                    TRAVODDS_LOG(LOG_ERROR, "DataSharing listener stopped: %s", error.what());
                } catch (...) {
                    TRAVODDS_LOG(LOG_ERROR, "DataSharing listener stopped by callback exception");
                }
                registration->close();
            });
        } catch (...) {
            nativeConnections_.erase(descriptor.writer);
            throw;
        }
        return true;
    } catch (const std::exception& error) {
        TRAVODDS_LOG(LOG_WARNING, "DataSharing worker unavailable; using RTPS: %s", error.what());
    }
#else
    (void)self; (void)writer;
#endif
    return false;
}

void Reader::StopDataSharing(const GUID_t& writer) {
    std::shared_ptr<NativeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(nativeConnectionsMutex_);
        auto it = nativeConnections_.find(writer);
        if (it == nativeConnections_.end()) return;
        connection = std::move(it->second);
        nativeConnections_.erase(it);
    }
#if defined(__linux__)
    connection->stop();
#endif
}
void Reader::StopDataSharing() {
    std::map<GUID_t, std::shared_ptr<NativeConnection>> connections;
    {
        std::lock_guard<std::mutex> lock(nativeConnectionsMutex_);
        nativeStopping_ = true;
        connections.swap(nativeConnections_);
    }
#if defined(__linux__)
    for (auto& connection : connections) connection.second->stop();
#endif
}

#if defined(__linux__)
ReturnCode_t Reader::ReceiveNativeSample(const GUID_t& writer, const NativeTypeInfo& type, SharedReadLoan&& loan) {
    if (!loan || loan.size() != type.size || !loan.sequence() ||
        loan.sequence() > uint64_t(std::numeric_limits<int64_t>::max())) return RETCODE_BAD_PARAMETER;
    std::lock_guard<std::recursive_mutex> ingress(dataMutex_);
    if (!datareader_ || !MatchedWriterIsMatched(writer)) return RETCODE_PRECONDITION_NOT_MET;
    const auto metadata = loan.metadata();
    InstanceHandle_t instance, related;
    std::memcpy(instance.keyHash.value, metadata.instance, 16);
    std::memcpy(related.keyHash.value, metadata.related_writer, 16);
    const auto sequence = loan.sequence();
    auto owner = std::make_shared<SharedReadLoan>(std::move(loan));
    auto cache = new_change(ALIVE, nullptr, instance, Conversion::ToInstanceHandle(writer),
        {metadata.source_seconds, metadata.source_nanoseconds});
    cache->nativePayload = std::shared_ptr<void>(owner, const_cast<char*>(owner->data()));
    cache->nativeType = type;
    cache->sequenceNumber = {int32_t(sequence >> 32), uint32_t(sequence)};
    cache->sourceTimespace = cache->sampleInfo.source_timestamp;
    cache->related_sample_identity.writer_handle = related;
    cache->related_sample_identity.sequence_number = {int32_t(metadata.related_sequence >> 32), uint32_t(metadata.related_sequence)};
    return SubmitDataChange(std::move(cache));
}
#endif
