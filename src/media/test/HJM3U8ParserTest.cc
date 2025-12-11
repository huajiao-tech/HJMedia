//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <gtest/gtest.h>
#include "HJM3U8Parser.h"

using namespace HJ;

//***********************************************************************************//
// 测试用M3U8内容
//***********************************************************************************//

// 基础Media Playlist
const char* kBasicMediaPlaylist = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:10
#EXT-X-MEDIA-SEQUENCE:0
#EXTINF:9.009,
segment0.ts
#EXTINF:9.009,
segment1.ts
#EXTINF:3.003,
segment2.ts
#EXT-X-ENDLIST
)";

// 带Discontinuity的Media Playlist
const char* kDiscontinuityPlaylist = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:10
#EXT-X-MEDIA-SEQUENCE:0
#EXT-X-DISCONTINUITY-SEQUENCE:5
#EXTINF:10.0,
part1_seg0.ts
#EXTINF:10.0,
part1_seg1.ts
#EXT-X-DISCONTINUITY
#EXTINF:8.0,
part2_seg0.ts
#EXTINF:8.0,
part2_seg1.ts
#EXTINF:8.0,
part2_seg2.ts
#EXT-X-DISCONTINUITY
#EXTINF:5.0,
part3_seg0.ts
#EXT-X-ENDLIST
)";

// Master Playlist (多分辨率)
const char* kMasterPlaylist = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-STREAM-INF:BANDWIDTH=1280000,RESOLUTION=1280x720,CODECS="avc1.640028,mp4a.40.2"
720p.m3u8
#EXT-X-STREAM-INF:BANDWIDTH=2560000,RESOLUTION=1920x1080,CODECS="avc1.640028,mp4a.40.2"
1080p.m3u8
#EXT-X-STREAM-INF:BANDWIDTH=640000,RESOLUTION=640x360,CODECS="avc1.4d001f,mp4a.40.2"
360p.m3u8
)";

// 带加密的Media Playlist
const char* kEncryptedPlaylist = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:10
#EXT-X-MEDIA-SEQUENCE:0
#EXT-X-KEY:METHOD=AES-128,URI="key.bin",IV=0x00000000000000000000000000000001
#EXTINF:10.0,
encrypted_seg0.ts
#EXTINF:10.0,
encrypted_seg1.ts
#EXT-X-ENDLIST
)";

//***********************************************************************************//
// 测试类
//***********************************************************************************//

class HJM3U8ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_parser = std::make_shared<HJM3U8Parser>();
    }

    void TearDown() override {
        m_parser->reset();
        m_parser = nullptr;
    }

    HJM3U8Parser::Ptr m_parser;
};

//***********************************************************************************//
// 基础解析测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, ParseBasicMediaPlaylist) {
    int res = m_parser->initFromContent(kBasicMediaPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto playlist = m_parser->getPlaylist();
    ASSERT_NE(playlist, nullptr);
    
    // 验证基本属性
    EXPECT_EQ(playlist->version, 3);
    EXPECT_EQ(playlist->targetDuration, 10);
    EXPECT_EQ(playlist->mediaSequence, 0);
    EXPECT_FALSE(playlist->isMasterPlaylist);
    EXPECT_TRUE(playlist->isVOD);
    
    // 验证分片数量
    EXPECT_EQ(playlist->getTotalSegmentCount(), 3);
    
    // 验证分组 (无discontinuity应该只有1个组)
    EXPECT_EQ(playlist->getGroupCount(), 1);
    
    // 验证总时长 (9.009 + 9.009 + 3.003 = 21.021秒)
    EXPECT_GE(playlist->totalDurationMs, 21000);
    EXPECT_LE(playlist->totalDurationMs, 22000);
}

TEST_F(HJM3U8ParserTest, ParseSegmentDetails) {
    int res = m_parser->initFromContent(kBasicMediaPlaylist, "http://example.com/hls/");
    ASSERT_EQ(res, HJ_OK);
    
    auto segments = m_parser->getPlaylist()->getAllSegments();
    ASSERT_EQ(segments.size(), 3);
    
    // 验证第一个分片
    EXPECT_EQ(segments[0].uri, "http://example.com/hls/segment0.ts");
    EXPECT_NEAR(segments[0].duration, 9.009, 0.001);
    EXPECT_EQ(segments[0].mediaSequence, 0);
    
    // 验证第二个分片
    EXPECT_EQ(segments[1].uri, "http://example.com/hls/segment1.ts");
    EXPECT_EQ(segments[1].mediaSequence, 1);
    
    // 验证第三个分片
    EXPECT_EQ(segments[2].uri, "http://example.com/hls/segment2.ts");
    EXPECT_NEAR(segments[2].duration, 3.003, 0.001);
}

//***********************************************************************************//
// Discontinuity分组测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, ParseDiscontinuityPlaylist) {
    int res = m_parser->initFromContent(kDiscontinuityPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto playlist = m_parser->getPlaylist();
    ASSERT_NE(playlist, nullptr);
    
    // 验证discontinuity sequence
    EXPECT_EQ(playlist->discontinuitySequence, 5);
    
    // 验证分组数量 (2个discontinuity = 3个组)
    EXPECT_EQ(playlist->getGroupCount(), 3);
    
    // 验证总分片数
    EXPECT_EQ(playlist->getTotalSegmentCount(), 6);
}

TEST_F(HJM3U8ParserTest, VerifySegmentGroups) {
    int res = m_parser->initFromContent(kDiscontinuityPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto& groups = m_parser->getSegmentGroups();
    ASSERT_EQ(groups.size(), 3);
    
    // 第一组: 2个分片
    EXPECT_EQ(groups[0]->segmentCount(), 2);
    EXPECT_EQ(groups[0]->groupIndex, 0);
    EXPECT_EQ(groups[0]->discontinuitySequence, 5);
    
    // 第二组: 3个分片
    EXPECT_EQ(groups[1]->segmentCount(), 3);
    EXPECT_EQ(groups[1]->groupIndex, 1);
    EXPECT_EQ(groups[1]->discontinuitySequence, 6);
    
    // 第三组: 1个分片
    EXPECT_EQ(groups[2]->segmentCount(), 1);
    EXPECT_EQ(groups[2]->groupIndex, 2);
    EXPECT_EQ(groups[2]->discontinuitySequence, 7);
}

TEST_F(HJM3U8ParserTest, VerifyDiscontinuityFlag) {
    int res = m_parser->initFromContent(kDiscontinuityPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto& groups = m_parser->getSegmentGroups();
    
    // 第二组第一个分片应该标记hasDiscontinuityBefore
    EXPECT_TRUE(groups[1]->segments[0].hasDiscontinuityBefore);
    
    // 第三组第一个分片应该标记hasDiscontinuityBefore
    EXPECT_TRUE(groups[2]->segments[0].hasDiscontinuityBefore);
    
    // 第一组第一个分片不应该标记
    EXPECT_FALSE(groups[0]->segments[0].hasDiscontinuityBefore);
}

//***********************************************************************************//
// Master Playlist测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, ParseMasterPlaylist) {
    int res = m_parser->initFromContent(kMasterPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto playlist = m_parser->getPlaylist();
    ASSERT_NE(playlist, nullptr);
    
    // 验证是Master Playlist
    EXPECT_TRUE(playlist->isMasterPlaylist);
    EXPECT_TRUE(m_parser->isMasterPlaylist());
    
    // 验证变体数量
    EXPECT_EQ(playlist->variants.size(), 3);
}

TEST_F(HJM3U8ParserTest, VerifyVariantDetails) {
    int res = m_parser->initFromContent(kMasterPlaylist, "http://example.com/hls/");
    ASSERT_EQ(res, HJ_OK);
    
    auto& variants = m_parser->getVariants();
    ASSERT_EQ(variants.size(), 3);
    
    // 验证720p变体
    EXPECT_EQ(variants[0]->bandwidth, 1280000);
    EXPECT_EQ(variants[0]->width, 1280);
    EXPECT_EQ(variants[0]->height, 720);
    EXPECT_EQ(variants[0]->uri, "http://example.com/hls/720p.m3u8");
    
    // 验证1080p变体
    EXPECT_EQ(variants[1]->bandwidth, 2560000);
    EXPECT_EQ(variants[1]->width, 1920);
    EXPECT_EQ(variants[1]->height, 1080);
    
    // 验证360p变体
    EXPECT_EQ(variants[2]->bandwidth, 640000);
    EXPECT_EQ(variants[2]->width, 640);
    EXPECT_EQ(variants[2]->height, 360);
}

TEST_F(HJM3U8ParserTest, SelectVariantByBandwidth) {
    int res = m_parser->initFromContent(kMasterPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    // 带宽足够高,选择最高码率
    int idx = m_parser->selectVariantByBandwidth(3000000);
    EXPECT_EQ(idx, 1); // 1080p (2560000)
    
    // 带宽中等
    idx = m_parser->selectVariantByBandwidth(1500000);
    EXPECT_EQ(idx, 0); // 720p (1280000)
    
    // 带宽较低
    idx = m_parser->selectVariantByBandwidth(800000);
    EXPECT_EQ(idx, 2); // 360p (640000)
    
    // 带宽极低,应选择最低码率
    idx = m_parser->selectVariantByBandwidth(100000);
    EXPECT_EQ(idx, 2); // 360p (最低)
}

//***********************************************************************************//
// 加密解析测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, ParseEncryptedPlaylist) {
    int res = m_parser->initFromContent(kEncryptedPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    auto segments = m_parser->getPlaylist()->getAllSegments();
    ASSERT_EQ(segments.size(), 2);
    
    // 验证加密信息
    EXPECT_EQ(segments[0].key.method, HJM3U8EncryptMethod::AES128);
    EXPECT_EQ(segments[0].key.uri, "http://example.com/key.bin");
    EXPECT_EQ(segments[0].key.iv, "0x00000000000000000000000000000001");
    EXPECT_TRUE(segments[0].key.isEncrypted());
}

//***********************************************************************************//
// 时间查找测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, FindSegmentByTime) {
    int res = m_parser->initFromContent(kBasicMediaPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    
    int groupIdx, segIdx;
    
    // 查找0秒位置 (应该在第一个分片)
    EXPECT_TRUE(m_parser->findSegmentByTime(0, groupIdx, segIdx));
    EXPECT_EQ(groupIdx, 0);
    EXPECT_EQ(segIdx, 0);
    
    // 查找10秒位置 (应该在第二个分片)
    EXPECT_TRUE(m_parser->findSegmentByTime(10000, groupIdx, segIdx));
    EXPECT_EQ(groupIdx, 0);
    EXPECT_EQ(segIdx, 1);
    
    // 查找19秒位置 (应该在第三个分片)
    EXPECT_TRUE(m_parser->findSegmentByTime(19000, groupIdx, segIdx));
    EXPECT_EQ(groupIdx, 0);
    EXPECT_EQ(segIdx, 2);
}

//***********************************************************************************//
// 错误处理测试
//***********************************************************************************//

TEST_F(HJM3U8ParserTest, InvalidContent) {
    // 空内容
    int res = m_parser->initFromContent("", "");
    EXPECT_NE(res, HJ_OK);
    
    // 无效M3U8 (缺少EXTM3U)
    res = m_parser->initFromContent("just some random text", "");
    EXPECT_NE(res, HJ_OK);
}

TEST_F(HJM3U8ParserTest, ResetParser) {
    int res = m_parser->initFromContent(kBasicMediaPlaylist, "http://example.com/");
    ASSERT_EQ(res, HJ_OK);
    ASSERT_NE(m_parser->getPlaylist(), nullptr);
    
    m_parser->reset();
    EXPECT_EQ(m_parser->getPlaylist(), nullptr);
    EXPECT_EQ(m_parser->getTotalDurationMs(), 0);
}
