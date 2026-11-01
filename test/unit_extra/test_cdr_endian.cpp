#include "gtest/gtest.h"
#include "cdr/travoddscdr.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

USING_TRAVODDS_NAMESPACE

namespace {

template<typename T>
using Bits = typename std::conditional<sizeof(T) == 2, uint16_t,
    typename std::conditional<sizeof(T) == 4, uint32_t, uint64_t>::type>::type;

template<typename T>
std::vector<Bits<T>> bitPatterns()
{
    if constexpr (sizeof(T) == 2) {
        return {0, 0xffff, 0x8000, 0x1234, 0x8123};
    } else if constexpr (sizeof(T) == 4) {
        // Signed zero, finite values, infinities, quiet NaN, subnormal and max.
        return {0, 0x80000000, 0x3f800000, 0xc0200000, 0x7f800000,
            0xff800000, 0x7fc12345, 1, 0x7f7fffff};
    } else {
        return {0, UINT64_C(0x8000000000000000), UINT64_C(0x3ff0000000000000),
            UINT64_C(0xc004000000000000), UINT64_C(0x7ff0000000000000),
            UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8123456789abc),
            1, UINT64_C(0x7fefffffffffffff)};
    }
}

template<typename T>
T fromBits(Bits<T> bits)
{
    T value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

template<typename T>
Bits<T> toBits(const T& value)
{
    Bits<T> bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

// Build expected CDR bytes independently of the serializer under test.
template<typename T>
std::vector<unsigned char> wireBytes(Bits<T> bits, bool littleEndian)
{
    std::vector<unsigned char> result(sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) {
        const size_t byte = littleEndian ? i : sizeof(T) - 1 - i;
        result[i] = static_cast<unsigned char>(bits >> (byte * 8));
    }
    return result;
}

void bindBuffer(SerializedBuffer& buffer, unsigned char* bytes, size_t size,
    bool readable = false)
{
    buffer.buffer = reinterpret_cast<char*>(bytes);
    buffer.origin_buffer = buffer.buffer;
    buffer.buffer_size = static_cast<uint32_t>(size);
    buffer.writer_position = readable ? buffer.buffer_size : 0;
}

template<typename T>
class CdrEndianTest : public ::testing::Test {};

using EndianTypes = ::testing::Types<float, double, int16_t, int32_t, int64_t>;
TYPED_TEST_SUITE(CdrEndianTest, EndianTypes);

TYPED_TEST(CdrEndianTest, SerializesScalarToKnownWireBytes)
{
    for (bool littleEndian : {false, true}) {
        SCOPED_TRACE(littleEndian);
        for (auto bits : bitPatterns<TypeParam>()) {
            SCOPED_TRACE(bits);
            std::array<unsigned char, sizeof(TypeParam)> bytes{};
            SerializedBuffer buffer;
            bindBuffer(buffer, bytes.data(), bytes.size());
            CdrSerializer serializer(&buffer, littleEndian);
            ASSERT_TRUE(serializer.serializeBaseType(fromBits<TypeParam>(bits)));
            EXPECT_EQ(buffer.writer_position, sizeof(TypeParam));
            EXPECT_EQ(std::vector<unsigned char>(bytes.begin(), bytes.end()),
                wireBytes<TypeParam>(bits, littleEndian));
        }
    }
}

TYPED_TEST(CdrEndianTest, DeserializesScalarFromKnownWireBytes)
{
    for (bool littleEndian : {false, true}) {
        SCOPED_TRACE(littleEndian);
        for (auto bits : bitPatterns<TypeParam>()) {
            SCOPED_TRACE(bits);
            auto bytes = wireBytes<TypeParam>(bits, littleEndian);
            SerializedBuffer buffer;
            bindBuffer(buffer, bytes.data(), bytes.size(), true);
            CdrDeserializer deserializer(&buffer, littleEndian);
            TypeParam value{};
            ASSERT_TRUE(deserializer.deserializeBaseType(value));
            EXPECT_EQ(toBits(value), bits);
            EXPECT_EQ(buffer.reader_position, sizeof(TypeParam));
        }
    }
}

TYPED_TEST(CdrEndianTest, SerializesAlignedArrayAcrossBuffers)
{
    for (bool littleEndian : {false, true}) {
        SCOPED_TRACE(littleEndian);
        std::vector<TypeParam> values;
        std::vector<unsigned char> expected(sizeof(TypeParam), 0);
        expected[0] = 0x55;
        for (auto bits : bitPatterns<TypeParam>()) {
            values.push_back(fromBits<TypeParam>(bits));
            const auto bytes = wireBytes<TypeParam>(bits, littleEndian);
            expected.insert(expected.end(), bytes.begin(), bytes.end());
        }
        std::vector<unsigned char> bytes(expected.size(), 0xaa);
        // The first scalar spans the buffer boundary, after alignment padding.
        const size_t split = sizeof(TypeParam) + 1;
        SerializedBuffer first, second;
        bindBuffer(first, bytes.data(), split);
        bindBuffer(second, bytes.data() + split, bytes.size() - split);
        first.next = &second;
        CdrSerializer serializer(&first, littleEndian);
        const uint8_t prefix = 0x55;
        ASSERT_TRUE(serializer.serializeBaseTypeArray(&prefix, 1));
        ASSERT_TRUE(serializer.serializeBaseTypeArray(values.data(),
            static_cast<uint32_t>(values.size())));
        EXPECT_EQ(bytes, expected);
        EXPECT_EQ(first.writer_position + second.writer_position, bytes.size());
    }
}

TYPED_TEST(CdrEndianTest, DeserializesAlignedArrayAcrossBuffers)
{
    for (bool littleEndian : {false, true}) {
        SCOPED_TRACE(littleEndian);
        const auto patterns = bitPatterns<TypeParam>();
        std::vector<unsigned char> bytes(sizeof(TypeParam), 0);
        bytes[0] = 0x55;
        for (auto bits : patterns) {
            const auto wire = wireBytes<TypeParam>(bits, littleEndian);
            bytes.insert(bytes.end(), wire.begin(), wire.end());
        }
        const size_t split = sizeof(TypeParam) + 1;
        SerializedBuffer first, second;
        bindBuffer(first, bytes.data(), split, true);
        bindBuffer(second, bytes.data() + split, bytes.size() - split, true);
        first.next = &second;
        CdrDeserializer deserializer(&first, littleEndian);
        uint8_t prefix = 0;
        ASSERT_TRUE(deserializer.deserializeBaseTypeArray(&prefix, 1));
        EXPECT_EQ(prefix, 0x55);
        std::vector<TypeParam> values(patterns.size());
        ASSERT_TRUE(deserializer.deserializeBaseTypeArray(values.data(),
            static_cast<uint32_t>(values.size())));
        for (size_t i = 0; i < values.size(); ++i) {
            SCOPED_TRACE(i);
            EXPECT_EQ(toBits(values[i]), patterns[i]);
        }
        EXPECT_EQ(first.reader_position + second.reader_position, bytes.size());
    }
}

} // namespace
