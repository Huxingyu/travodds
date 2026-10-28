// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#pragma once
#include "dcps/topic/typesupport.h"
#include <atomic>

namespace datasharing_demo {
using namespace TRAVODDS;

// Fixed-size, pointer-free, with the same versioned ABI in both processes.
struct alignas(64) Sample {
    uint64_t key;
    uint64_t sequence;
    uint64_t payload[6];
};

class SampleSupport : public NativeTypeSupport<Sample> {
public:
    explicit SampleSupport(bool keyed);
    std::atomic<bool> blockSerialization{true}, blockDeserialization{true};
    std::atomic<unsigned> serializations{0}, deserializations{0};
    void resetCounts();
    void* create_data() override;
    void delete_data(void*) override;
    int32_t copy_data(void*, void*) override;
    uint32_t get_serialized_data_size(void*, uint32_t) override { return sizeof(Sample); }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return sizeof(Sample); }
    int32_t serialize_data(void*, CdrSerializer*, int32_t) override;
    int32_t deserialize_data(void*, CdrDeserializer*, int32_t) override;
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void*, CdrSerializer*, InstanceHandle_t&, bool) override;
    int32_t MakeKey(const void*, InstanceHandle_t&, bool) override;
    bool has_key() override { return keyed_; }
    const char* get_typename() override;
private:
    bool keyed_;
};
Sample sample(uint64_t key, uint64_t sequence);
bool samePayload(const Sample& value, uint64_t key, uint64_t sequence);
}
