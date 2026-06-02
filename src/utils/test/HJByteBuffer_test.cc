#include <gtest/gtest.h>
#include "HJByteBuffer.h"

NS_HJ_USING;

TEST(HJJsonReflectionTest, HJByteBufferIntegralRoundTrip) {
    HJByteBuffer buffer;

    buffer.write<uint16_t>(0x1234U);
    buffer.write<int32_t>(0x12345678);
    buffer.write<uint64_t>(0x0102030405060708ULL);

    EXPECT_EQ(buffer.size(), sizeof(uint16_t) + sizeof(int32_t) + sizeof(uint64_t));
    EXPECT_EQ(buffer.read<uint16_t>(), 0x1234U);
    EXPECT_EQ(buffer.read<int32_t>(), 0x12345678);
    EXPECT_EQ(buffer.read<uint64_t>(), 0x0102030405060708ULL);
    EXPECT_TRUE(buffer.eof());
}

TEST(HJJsonReflectionTest, HJByteBufferReadWrite24KeepsBigEndianBytes) {
    HJByteBuffer buffer;

    buffer.write24(0x112233U);

    ASSERT_EQ(buffer.size(), 3U);
    EXPECT_EQ(buffer.data()[0], 0x11U);
    EXPECT_EQ(buffer.data()[1], 0x22U);
    EXPECT_EQ(buffer.data()[2], 0x33U);
    EXPECT_EQ(buffer.read24(), 0x112233U);
}

TEST(HJJsonReflectionTest, HJByteBufferRejectsOverflow24BitValue) {
    HJByteBuffer buffer;

    EXPECT_THROW(buffer.write24(0x01000000U), HJInvalidParametersException);
}

TEST(HJJsonReflectionTest, HJByteBufferValidatesBytePointers) {
    HJByteBuffer write_buffer;
    HJByteBuffer read_buffer;
    const uint8_t source[] = {0xAAU, 0xBBU, 0xCCU};

    EXPECT_NO_THROW(write_buffer.writeBytes(nullptr, 0));
    EXPECT_THROW(write_buffer.writeBytes(nullptr, 1), HJInvalidParametersException);

    read_buffer.writeBytes(source, sizeof(source));
    EXPECT_NO_THROW(read_buffer.readBytes(nullptr, 0));
    EXPECT_THROW(read_buffer.readBytes(nullptr, 1), HJInvalidParametersException);
}

TEST(HJJsonReflectionTest, HJByteBufferReadOutOfRangeThrows) {
    HJByteBuffer buffer;

    buffer.write<uint16_t>(0xBEEFU);
    EXPECT_EQ(buffer.read<uint16_t>(), 0xBEEFU);
    EXPECT_THROW(buffer.read<uint8_t>(), HJInvalidCallException);
}

TEST(HJJsonReflectionTest, HJByteBufferResetAndClearUpdateState) {
    HJByteBuffer buffer;

    buffer.write<uint8_t>(0x12U);
    buffer.write<uint8_t>(0x34U);

    EXPECT_EQ(buffer.read<uint8_t>(), 0x12U);
    EXPECT_EQ(buffer.remaining(), 1U);

    buffer.reset();
    EXPECT_EQ(buffer.read<uint8_t>(), 0x12U);

    buffer.clear();
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_EQ(buffer.remaining(), 0U);
    EXPECT_TRUE(buffer.eof());
}
