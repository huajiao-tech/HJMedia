#include "gtest/gtest.h"
#include "../sei/HJSEIWrapper.h"
#include <cstring>

NS_HJ_USING;

namespace {
static std::vector<uint8_t> makeBytes(const char* s) {
    return std::vector<uint8_t>(s, s + std::strlen(s));
}
static bool hasStartCode4(const std::vector<uint8_t>& v) {
    return v.size() >= 4 && v[0]==0x00 && v[1]==0x00 && v[2]==0x00 && v[3]==0x01;
}
}

TEST(HJSEITest, PackAndParse_H264_AnnexB_Single)
{
    HJSEIData d;
    d.uuid = "00112233445566778899aabbccddeeff"; // hex uuid
    d.data = makeBytes("hello");

    auto out = HJSEIWrapper::makeSEINal({d}, /*isH265=*/false);
    ASSERT_FALSE(out.empty());
    ASSERT_TRUE(hasStartCode4(out));

    // H.264 header after start code should be 0x06 (SEI)
    ASSERT_GE(out.size(), 5u);
    EXPECT_EQ(out[4] & 0x1F, 6);

    auto parsed = HJSEIWrapper::parseSEINals(out);
    ASSERT_EQ(parsed.size(), 1u);
    EXPECT_EQ(parsed[0].uuid, "00112233445566778899aabbccddeeff");
    EXPECT_EQ(std::string(parsed[0].data.begin(), parsed[0].data.end()), "hello");
}

TEST(HJSEITest, PackMerged_H264_AnnexB_TwoMessages)
{
    HJSEIData d1{ "00112233445566778899aabbccddeeff", makeBytes("hello") };
    HJSEIData d2{ "ffeeddccbbaa99887766554433221100", makeBytes("world") };

    auto out = HJSEIWrapper::makeSEINalMerged({d1, d2}, /*isH265=*/false, HJSEIWrapper::HJNALFormat::AnnexB);
    ASSERT_FALSE(out.empty());
    ASSERT_TRUE(hasStartCode4(out));
    ASSERT_GE(out.size(), 5u);
    EXPECT_EQ(out[4] & 0x1F, 6);

    auto parsed = HJSEIWrapper::parseSEINals(out);
    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed[0].uuid, d1.uuid);
    EXPECT_EQ(std::string(parsed[0].data.begin(), parsed[0].data.end()), "hello");
    EXPECT_EQ(parsed[1].uuid, d2.uuid);
    EXPECT_EQ(std::string(parsed[1].data.begin(), parsed[1].data.end()), "world");
}

TEST(HJSEITest, Pack_H265_AnnexB_Single)
{
    HJSEIData d;
    d.uuid = "00112233445566778899aabbccddeeff";
    d.data = makeBytes("h265");

    auto out = HJSEIWrapper::makeSEINal({d}, /*isH265=*/true);
    ASSERT_FALSE(out.empty());
    ASSERT_TRUE(hasStartCode4(out));
    ASSERT_GE(out.size(), 6u);
    // H.265 nal type is in bits[1..6] of first header byte
    uint8_t type = (out[4] >> 1) & 0x3F;
    EXPECT_EQ(type, 39); // prefix SEI

    auto parsed = HJSEIWrapper::parseSEINals(out);
    ASSERT_EQ(parsed.size(), 1u);
    EXPECT_EQ(parsed[0].uuid, "00112233445566778899aabbccddeeff");
    EXPECT_EQ(std::string(parsed[0].data.begin(), parsed[0].data.end()), "h265");
}

TEST(HJSEITest, Pack_AVCC_LengthPrefixed)
{
    HJSEIData d;
    d.uuid = "00112233445566778899aabbccddeeff";
    d.data = makeBytes("avcc");

    // H.264 AVCC
    auto out264 = HJSEIWrapper::makeSEINal({d}, /*isH265=*/false, HJSEIWrapper::HJNALFormat::AVCC);
    ASSERT_GE(out264.size(), 5u);
    // first 4 bytes are length
    uint32_t len264 = (out264[0]<<24) | (out264[1]<<16) | (out264[2]<<8) | out264[3];
    EXPECT_EQ(len264 + 4, out264.size());
    EXPECT_EQ(out264[4] & 0x1F, 6);

    // H.265 AVCC
    auto out265 = HJSEIWrapper::makeSEINal({d}, /*isH265=*/true, HJSEIWrapper::HJNALFormat::AVCC);
    ASSERT_GE(out265.size(), 6u);
    uint32_t len265 = (out265[0]<<24) | (out265[1]<<16) | (out265[2]<<8) | out265[3];
    EXPECT_EQ(len265 + 4, out265.size());
    uint8_t type = (out265[4] >> 1) & 0x3F;
    EXPECT_EQ(type, 39);
}

