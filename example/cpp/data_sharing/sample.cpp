// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "sample.h"

namespace datasharing_demo {
static_assert(sizeof(Sample) == 64, "Update the layout ID when the sample ABI changes");
SampleSupport::SampleSupport(bool keyed)
    : NativeTypeSupport(keyed ? "travodds.demo.v1.key-u64.seq-u64.payload-u64x6.align64" :
        "travodds.demo.v1.no-key.seq-u64.payload-u64x6.align64"), keyed_(keyed) {}
void SampleSupport::resetCounts() { serializations = 0; deserializations = 0; }
void* SampleSupport::create_data() { return new Sample{}; }
void SampleSupport::delete_data(void* p) { delete static_cast<Sample*>(p); }
int32_t SampleSupport::copy_data(void* dst, void* src) {
    *static_cast<Sample*>(dst) = *static_cast<Sample*>(src); return 0;
}
int32_t SampleSupport::serialize_data(void* p, CdrSerializer* cdr, int32_t) {
    ++serializations;
    if (blockSerialization) return -1;
    const auto& value = *static_cast<Sample*>(p);
    if (!cdr->serializeBaseType(value.key) || !cdr->serializeBaseType(value.sequence)) return -1;
    for (auto field : value.payload) if (!cdr->serializeBaseType(field)) return -1;
    return 0;
}
int32_t SampleSupport::deserialize_data(void* p, CdrDeserializer* cdr, int32_t) {
    ++deserializations;
    if (blockDeserialization) return -1;
    auto& value = *static_cast<Sample*>(p);
    if (!cdr->deserializeBaseType(value.key) || !cdr->deserializeBaseType(value.sequence)) return -1;
    for (auto& field : value.payload) if (!cdr->deserializeBaseType(field)) return -1;
    return 0;
}
ReturnCode_t SampleSupport::get_instancehandle(void* p, CdrSerializer* cdr,
        InstanceHandle_t& handle, bool md5) {
    handle = HANDLE_NIL;
    if (!keyed_) return RETCODE_OK;
    if (!cdr->serializeBaseType(static_cast<Sample*>(p)->key) ||
        !cdr->getKeyHash(reinterpret_cast<char*>(&handle), md5)) return RETCODE_ERROR;
    return RETCODE_OK;
}
int32_t SampleSupport::MakeKey(const void* p, InstanceHandle_t& handle, bool md5) {
    char storage[16]{};
    SerializedBuffer buffer; buffer.buffer = storage; buffer.buffer_size = sizeof(storage);
    CdrSerializer cdr(&buffer);
    return get_instancehandle(const_cast<void*>(p), &cdr, handle, md5) == RETCODE_OK ? 0 : -1;
}
const char* SampleSupport::get_typename() {
    return keyed_ ? "DataSharingDemoKeyed" : "DataSharingDemoPlain";
}
Sample sample(uint64_t key, uint64_t sequence) {
    Sample value{}; value.key = key; value.sequence = sequence;
    for (unsigned i = 0; i < 6; ++i) value.payload[i] = key * 1000 + sequence * 10 + i;
    return value;
}
bool samePayload(const Sample& value, uint64_t key, uint64_t sequence) {
    const auto expected = sample(key, sequence);
    if (value.key != key || value.sequence != sequence) return false;
    for (unsigned i = 0; i < 6; ++i) if (value.payload[i] != expected.payload[i]) return false;
    return true;
}
}
