// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_COMMON_MEMORY_SHAREDPAYLOADPOOL_H
#define TRAVODDS_COMMON_MEMORY_SHAREDPAYLOADPOOL_H

#include "common/memory/payloadpool.h"
#include "type/guid.h"
#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

#if defined(__linux__)
TRAVODDS_NAMESPACE_BEGIN

struct SharedPoolConfig {
    uint32_t payload_size = 64;
    uint32_t slots = 64;
    uint32_t max_readers = 16;
    uint32_t alignment = 64;
    uint64_t max_segment_bytes = uint64_t(1) << 30;
};

struct SharedPoolSession {
    uint64_t high = 0;
    uint64_t low = 0;
    bool operator==(const SharedPoolSession& other) const {
        return high == other.high && low == other.low;
    }
};

struct SharedPoolDescriptor {
    std::string path;
    GUID_t writer;
    SharedPoolSession session;
    SharedPoolConfig config;
    uint64_t segment_bytes = 0;
};

struct SharedSampleToken {
    SharedPoolSession session;
    uint32_t slot = 0;
    uint64_t generation = 0;
};

// Pointer-free DDS metadata delivered with the native bytes. Sequence numbers
// use the positive RTPS 64-bit representation; no process-local objects appear
// in the mapped slot.
struct SharedSampleMetadata {
    int32_t source_seconds = 0;
    uint32_t source_nanoseconds = 0;
    unsigned char instance[16]{};
    unsigned char related_writer[16]{};
    uint64_t related_sequence = 0;
};

enum class SharedPoolResult { OK, EXHAUSTED, TIMEOUT, CLOSED, INVALID, CORRUPT, SYSTEM_ERROR };

struct SharedPoolStats {
    uint32_t free_slots = 0;
    uint32_t writing_slots = 0;
    uint32_t published_slots = 0;
    uint32_t readers = 0;
    uint64_t pending_references = 0;
    uint64_t loan_references = 0;
    bool writer_alive = false;
};

class SharedPayloadPool;
class SharedPoolReader;

// A move-only pin on mapped storage. Retains the registration and mapping;
// neither Writer closure nor dropping the Reader handle invalidates the bytes.
class SharedReadLoan {
public:
    SharedReadLoan() = default;
    ~SharedReadLoan();
    SharedReadLoan(SharedReadLoan&& other) noexcept;
    SharedReadLoan& operator=(SharedReadLoan&& other) noexcept;
    SharedReadLoan(const SharedReadLoan&) = delete;
    SharedReadLoan& operator=(const SharedReadLoan&) = delete;
    explicit operator bool() const { return static_cast<bool>(reader_); }
    const char* data() const { return data_; }
    uint32_t size() const { return size_; }
    uint64_t sequence() const { return sequence_; }
    const SharedSampleMetadata& metadata() const { return metadata_; }
    const SharedSampleToken& token() const { return token_; }
    void reset();
private:
    std::shared_ptr<SharedPoolReader> reader_;
    SharedSampleToken token_;
    const char* data_ = nullptr;
    uint32_t size_ = 0;
    uint64_t sequence_ = 0;
    SharedSampleMetadata metadata_;
    friend class SharedPayloadPool;
    friend class SharedPoolReader;
};

// Destruction drops unread deliveries; active loans keep registration alive.
class SharedPoolReader : public std::enable_shared_from_this<SharedPoolReader> {
public:
    ~SharedPoolReader();
    SharedPoolReader(const SharedPoolReader&) = delete;
    SharedPoolReader& operator=(const SharedPoolReader&) = delete;
    SharedPoolResult waitNext(uint64_t afterSequence, std::chrono::milliseconds timeout, SharedReadLoan& output);
    bool returnLoan(SharedReadLoan& loan);
    // Acknowledge an unread delivery by token (e.g. Reader history filtering).
    bool discard(const SharedSampleToken& sample);
    // Stop future/unread deliveries without invalidating existing application
    // Loans. Their registration and mapping remain pinned until final return.
    void close();
private:
    SharedPoolReader(std::shared_ptr<SharedPayloadPool> pool, uint32_t index, uint64_t generation);
    std::shared_ptr<SharedPayloadPool> pool_;
    uint32_t index_;
    uint64_t generation_;
    bool registered_ = false;
    friend class SharedPayloadPool;
    friend class SharedReadLoan;
};

// Linux implementation: a preallocated native-layout Writer segment, mapped
// independently by Readers. No C++ pointers/containers are stored in the file.
class SharedPayloadPool : public IPayloadPool, public std::enable_shared_from_this<SharedPayloadPool> {
public:
    static std::shared_ptr<SharedPayloadPool> create(const GUID_t& writer,
        const SharedPoolConfig& config, const std::string& directory = "/dev/shm");
    static std::shared_ptr<SharedPayloadPool> open(const SharedPoolDescriptor& descriptor);
    static std::string pathFor(const GUID_t& writer, const SharedPoolSession& session,
        const std::string& directory);
    // Only removes the exact session/inode after proven Writer process death.
    static bool removeOrphan(const SharedPoolDescriptor& descriptor);
    ~SharedPayloadPool() override;
    SharedPayloadPool(const SharedPayloadPool&) = delete;
    SharedPayloadPool& operator=(const SharedPayloadPool&) = delete;

    const SharedPoolDescriptor& descriptor() const { return descriptor_; }
    SharedPoolResult loan(uint32_t size, SharedSampleToken& token, char*& buffer,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    SharedPoolResult publish(const SharedSampleToken& token, uint32_t size, uint64_t sequence,
        const SharedSampleMetadata& metadata = {}, const std::vector<GUID_t>* readers = nullptr);
    bool releaseWriter(const SharedSampleToken& token);
    std::shared_ptr<SharedPoolReader> registerReader(const GUID_t& reader = GUID_UNKNOWN, bool replay = false);
    // replayOnly selects registrations eligible for retained history.
    std::vector<GUID_t> registeredReaders(bool replayOnly = false);
    SharedPoolResult statistics(SharedPoolStats& stats);
    // Raw IPayloadPool ownership is for internal history only. User Loan APIs
    // must retain and validate tokens, since addresses alone cannot detect ABA.
    bool getBuffer(uint32_t size, char*& buffer) override;
    bool releaseBuffer(char*& buffer) override;
    void closeWriter();
private:
    struct Impl;
    SharedPayloadPool() = default;
    SharedPoolResult waitNext(const std::shared_ptr<SharedPoolReader>& reader,
        uint64_t after, std::chrono::milliseconds timeout, SharedReadLoan& output);
    bool releaseReader(const SharedPoolReader& reader, const SharedSampleToken& token, bool unread);
    void unregisterReader(const SharedPoolReader& reader);
    void closeReader(const SharedPoolReader& reader);
    void unlinkOwnedFile();
    SharedPoolDescriptor descriptor_;
    std::unique_ptr<Impl> impl_;
    friend class SharedPoolReader;
    friend class SharedReadLoan;
};

TRAVODDS_NAMESPACE_END
#endif
#endif
