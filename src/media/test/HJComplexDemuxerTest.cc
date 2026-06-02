//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <gtest/gtest.h>
#include "HJComplexDemuxer.h"
#include "HJMediaFrame.h"
#include "HJFLog.h"

using namespace HJ;

#define HJ_DEMUXER_TEST 0
//***********************************************************************************//
// 测试素材路径
//***********************************************************************************//
namespace {
    // 3个ts分片的m3u8
    const std::string kSampleM3U8 = "E:/movies/replaym3u8/index_sample.m3u8";
    // 带discontinuity的m3u8
    const std::string kDiscontinuityM3U8 = "https://file-22.huajiao.com/record/main/HJ_0_ali_1_main__h265_271393148_1765772819219_8223_O/replay.m3u8"; //"E:/movies/replaym3u8/index_discontinuity.m3u8";
    // 527个分片带discontinuity的m3u8
    const std::string kLongM3U8 = "E:/movies/replaym3u8/index_long.m3u8";
    // 空的m3u8
    const std::string kEmptyM3U8 = "E:/movies/replaym3u8/index_empty.m3u8";
}

//***********************************************************************************//
// 测试类 Fixture
//***********************************************************************************//
class HJComplexDemuxerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_demuxer = std::make_shared<HJComplexDemuxer>();
    }

    void TearDown() override {
        if (m_demuxer) {
            m_demuxer->done();
            m_demuxer = nullptr;
        }
    }

    // 辅助方法: 创建MediaUrl
    HJMediaUrl::Ptr createM3U8Url(const std::string& path, int loopCnt = 1) {
        auto url = std::make_shared<HJMediaUrl>(path, loopCnt);
        return url;
    }

    // 辅助方法: 读取所有帧并返回帧数
    int readAllFrames(bool printProgress = false) {
        int frameCount = 0;
        int videoCount = 0;
        int audioCount = 0;
        int64_t lastPts = -1;
        
        while (true) {
            HJMediaFrame::Ptr frame = nullptr;
            int res = m_demuxer->getFrame(frame);
            
            if (res == HJ_EOF) {
                if (printProgress) {
                    HJFLogi("EOF reached, total frames: {}", frameCount);
                }
                break;
            }
            if (res < HJ_OK || !frame) {
                if (printProgress) {
                    HJFLoge("getFrame failed: {}", res);
                }
                break;
            }
            
            frameCount++;
            if (frame->isVideo()) {
                videoCount++;
            } else if (frame->isAudio()) {
                audioCount++;
            }
            
            // 检查时间戳递增
            int64_t pts = frame->getPTS();
            if (lastPts >= 0 && pts < lastPts) {
                HJFLogw("PTS regression detected: last={}, current={}", lastPts, pts);
            }
            lastPts = pts;
            
            if (printProgress && frameCount % 100 == 0) {
                HJFLogi("Read {} frames (V:{}, A:{}), pts={}ms", 
                    frameCount, videoCount, audioCount, pts);
            }
        }
        
        if (printProgress) {
            HJFLogi("Total: {} frames (Video:{}, Audio:{})", 
                frameCount, videoCount, audioCount);
        }
        return frameCount;
    }

    // 辅助方法: 读取指定数量的帧
    int readFrames(int count) {
        int frameCount = 0;
        while (frameCount < count) {
            HJMediaFrame::Ptr frame = nullptr;
            int res = m_demuxer->getFrame(frame);
            if (res == HJ_EOF || res < HJ_OK || !frame) {
                break;
            }
            frameCount++;
        }
        return frameCount;
    }

    HJComplexDemuxer::Ptr m_demuxer;
};

#if HJ_DEMUXER_TEST
//***********************************************************************************//
// 一、初始化测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Init_SampleM3U8_Success) {
    auto url = createM3U8Url(kSampleM3U8);
    int res = m_demuxer->init(url);
    
    ASSERT_EQ(res, HJ_OK) << "Failed to init with sample m3u8";
    EXPECT_TRUE(m_demuxer->isReady());
    EXPECT_GT(m_demuxer->getDuration(), 0) << "Duration should be > 0";
    
    auto& mediaInfo = m_demuxer->getMediaInfo();
    ASSERT_NE(mediaInfo, nullptr);
    HJFLogi("Sample M3U8 Duration: {}ms", m_demuxer->getDuration());
}

TEST_F(HJComplexDemuxerTest, Init_DiscontinuityM3U8_Success) {
    auto url = createM3U8Url(kDiscontinuityM3U8);
    int res = m_demuxer->init(url);
    
    ASSERT_EQ(res, HJ_OK) << "Failed to init with discontinuity m3u8";
    EXPECT_TRUE(m_demuxer->isReady());
    EXPECT_GT(m_demuxer->getDuration(), 0);
    
    HJFLogi("Discontinuity M3U8 Duration: {}ms", m_demuxer->getDuration());
}

TEST_F(HJComplexDemuxerTest, Init_LongM3U8_Success) {
    auto url = createM3U8Url(kLongM3U8);
    int res = m_demuxer->init(url);
    
    ASSERT_EQ(res, HJ_OK) << "Failed to init with long m3u8 (527 segments)";
    EXPECT_TRUE(m_demuxer->isReady());
    EXPECT_GT(m_demuxer->getDuration(), 0);
    
    HJFLogi("Long M3U8 Duration: {}ms, segments: 527", m_demuxer->getDuration());
}

TEST_F(HJComplexDemuxerTest, Init_EmptyM3U8_ReturnsError) {
    auto url = createM3U8Url(kEmptyM3U8);
    int res = m_demuxer->init(url);
    
    // 空m3u8应该返回错误或者无法正常播放
    // 根据HJHLSParser的实现，可能返回HJErrInvalid或其他错误
    if (res == HJ_OK) {
        // 如果init成功，但duration应该为0或无法读取帧
        int64_t duration = m_demuxer->getDuration();
        HJFLogw("Empty M3U8 init succeeded, duration: {}", duration);
        
        // 尝试读帧，应该立即返回EOF
        HJMediaFrame::Ptr frame = nullptr;
        int frameRes = m_demuxer->getFrame(frame);
        EXPECT_TRUE(frameRes == HJ_EOF || frameRes < HJ_OK || !frame);
    } else {
        HJFLogi("Empty M3U8 init failed as expected: {}", res);
        EXPECT_NE(res, HJ_OK);
    }
}

TEST_F(HJComplexDemuxerTest, Init_NullUrl_ReturnsError) {
    HJMediaUrl::Ptr nullUrl = nullptr;
    int res = m_demuxer->init(nullUrl);
    
    EXPECT_EQ(res, HJErrInvalidParams);
}

TEST_F(HJComplexDemuxerTest, Init_InvalidPath_ReturnsError) {
    auto url = createM3U8Url("E:/nonexistent/invalid.m3u8");
    int res = m_demuxer->init(url);
    
    EXPECT_NE(res, HJ_OK) << "Should fail with invalid path";
}

//***********************************************************************************//
// 二、GetFrame测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, GetFrame_SampleM3U8_ReadAllFrames) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int frameCount = readAllFrames(true);
    EXPECT_GT(frameCount, 0) << "Should read at least some frames";
    
    HJFLogi("Sample M3U8 total frames: {}", frameCount);
}

TEST_F(HJComplexDemuxerTest, GetFrame_DiscontinuityM3U8_SeamlessTransition) {
    auto url = createM3U8Url(kDiscontinuityM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int frameCount = 0;
    int64_t prevPts = -1;
    int ptsRegressionCount = 0;
    
    while (true) {
        HJMediaFrame::Ptr frame = nullptr;
        int res = m_demuxer->getFrame(frame);
        
        if (res == HJ_EOF || res < HJ_OK || !frame) {
            break;
        }
        
        frameCount++;
        int64_t pts = frame->getPTS();
        
        // 检查discontinuity时是否有PTS回退（在timeOffsetEnable关闭时可能发生）
        if (prevPts >= 0 && pts < prevPts - 1000) { // 允许1秒的误差
            ptsRegressionCount++;
            HJFLogw("Discontinuity detected at frame {}: pts {} -> {}", 
                frameCount, prevPts, pts);
        }
        prevPts = pts;
    }
    
    EXPECT_GT(frameCount, 0);
    HJFLogi("Discontinuity M3U8: {} frames, {} pts regressions", 
        frameCount, ptsRegressionCount);
}

TEST_F(HJComplexDemuxerTest, GetFrame_BeforeInit_ReturnsError) {
    // 不调用init，直接getFrame
    HJMediaFrame::Ptr frame = nullptr;
    int res = m_demuxer->getFrame(frame);
    
    EXPECT_EQ(res, HJErrNotAlready);
    EXPECT_EQ(frame, nullptr);
}

TEST_F(HJComplexDemuxerTest, GetFrame_LongM3U8_FirstSegments) {
    auto url = createM3U8Url(kLongM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // 只读取前1000帧，避免测试时间过长
    int frameCount = readFrames(1000);
    EXPECT_GE(frameCount, 100) << "Should read at least 100 frames from long m3u8";
    
    HJFLogi("Long M3U8 first {} frames read successfully", frameCount);
}

//***********************************************************************************//
// 三、Seek测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Seek_ToStart_Success) {
    auto url = createM3U8Url(kSampleM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // 先读一些帧
    readFrames(50);
    
    // seek到开始
    int res = m_demuxer->seek(0);
    EXPECT_EQ(res, HJ_OK);
    
    // 读取第一帧
    HJMediaFrame::Ptr frame = nullptr;
    res = m_demuxer->getFrame(frame);
    EXPECT_EQ(res, HJ_OK);
    ASSERT_NE(frame, nullptr);
    
    // 第一帧的PTS应该接近0
    int64_t pts = frame->getPTS();
    EXPECT_LT(pts, 1000) << "After seek(0), first frame PTS should be near 0";
    HJFLogi("After seek(0), first frame pts: {}ms", pts);
}

TEST_F(HJComplexDemuxerTest, Seek_MidPosition_Success) {
    auto url = createM3U8Url(kSampleM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    ASSERT_GT(duration, 0);
    
    // seek到中间位置
    int64_t seekPos = duration / 2;
    int res = m_demuxer->seek(seekPos);
    EXPECT_EQ(res, HJ_OK);
    
    // 读取帧
    HJMediaFrame::Ptr frame = nullptr;
    res = m_demuxer->getFrame(frame);
    EXPECT_EQ(res, HJ_OK);
    ASSERT_NE(frame, nullptr);
    
    int64_t pts = frame->getPTS();
    HJFLogi("Seek to {}ms, got frame at {}ms", seekPos, pts);
    
    // PTS应该在seek位置附近（允许一定误差，因为seek到关键帧）
    EXPECT_GE(pts, seekPos - 5000);  // 允许向前5秒误差
    EXPECT_LE(pts, seekPos + 5000);  // 允许向后5秒误差
}

TEST_F(HJComplexDemuxerTest, Seek_NegativePos_ClampToZero) {
    auto url = createM3U8Url(kSampleM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // seek到负数位置
    int res = m_demuxer->seek(-1000);
    EXPECT_EQ(res, HJ_OK);
    
    // 读取帧，应该从开始
    HJMediaFrame::Ptr frame = nullptr;
    res = m_demuxer->getFrame(frame);
    EXPECT_EQ(res, HJ_OK);
    ASSERT_NE(frame, nullptr);
    
    int64_t pts = frame->getPTS();
    EXPECT_LT(pts, 500) << "After seek(-1000), should start from beginning";
}

TEST_F(HJComplexDemuxerTest, Seek_ExceedsDuration_SeekToEnd) {
    auto url = createM3U8Url(kSampleM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    
    // seek到超过duration的位置
    int res = m_demuxer->seek(duration + 10000);
    EXPECT_EQ(res, HJ_OK);
    
    // 读取帧
    HJMediaFrame::Ptr frame = nullptr;
    res = m_demuxer->getFrame(frame);
    
    // 可能返回EOF或者最后一帧
    int64_t pts = frame->getPTS();
    EXPECT_LT(pts, duration) << "After to end (duration + 10000), should start from beginning";
    HJFLogi("Seek beyond duration: res={}, pts:{}, duration:{}", res, pts, duration);
}

TEST_F(HJComplexDemuxerTest, Seek_BeforeInit_ReturnsError) {
    int res = m_demuxer->seek(1000);
    EXPECT_EQ(res, HJErrNotAlready);
}

TEST_F(HJComplexDemuxerTest, Seek_CrossSegment_DiscontinuityM3U8) {
    auto url = createM3U8Url(kDiscontinuityM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    ASSERT_GT(duration, 0);
    
    // 尝试seek到不同位置
    std::vector<int64_t> seekPositions = {0, duration/4, duration/2, duration*3/4};
    
    for (int64_t pos : seekPositions) {
        int res = m_demuxer->seek(pos);
        EXPECT_EQ(res, HJ_OK) << "Seek to " << pos << "ms failed";
        
        HJMediaFrame::Ptr frame = nullptr;
        res = m_demuxer->getFrame(frame);
        if (res == HJ_OK && frame) {
            int64_t pts = frame->getPTS();
            HJFLogi("Seek to {}ms, got frame at {}ms", pos, pts);
        }
    }
}
#endif //HJ_DEMUXER_TEST

TEST_F(HJComplexDemuxerTest, Seek_CrossSegment_DiscontinuityM3U8_rand) {
    auto url = createM3U8Url(kDiscontinuityM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);

    int64_t duration = m_demuxer->getDuration();
    ASSERT_GT(duration, 0);

    // 尝试seek到不同位置
    //std::vector<int64_t> seekPositions = { 0, duration / 4, duration / 2, duration * 3 / 4 };

    for (int i = 0; i < 100; i++) {
        int64_t pos = HJUtilitys::random(0, duration);
        int res = m_demuxer->seek(pos);
        EXPECT_EQ(res, HJ_OK) << "Seek to " << pos << "ms failed";

        HJMediaFrame::Ptr frame = nullptr;
        res = m_demuxer->getFrame(frame);
        if (res == HJ_OK && frame) {
            int64_t pts = frame->getPTS();
            HJFLogi("Seek to {}ms, got {}ms, delta:{}", pos, pts, pos - pts);
        }
    }
}

#if HJ_DEMUXER_TEST
TEST_F(HJComplexDemuxerTest, Seek_LongM3U8_ToMiddle) {
    auto url = createM3U8Url(kLongM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    HJFLogi("Long M3U8 duration: {}ms", duration);
    
    // seek到中间
    int64_t seekPos = duration / 2;
    int res = m_demuxer->seek(seekPos);
    ASSERT_EQ(res, HJ_OK);
    
    // 读取一些帧
    int frameCount = readFrames(100);
    EXPECT_GT(frameCount, 0) << "Should read frames after seek to middle";
    HJFLogi("After seek to {}ms, read {} frames", seekPos, frameCount);
}

//***********************************************************************************//
// 四、Reset测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Reset_AfterRead_BackToStart) {
    auto url = createM3U8Url(kSampleM3U8);
    m_demuxer->setTimeOffsetEnable(true);

    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // 读取一些帧
    readFrames(100);
    
    // reset
    m_demuxer->reset();
    
    // 读取第一帧
    HJMediaFrame::Ptr frame = nullptr;
    int res = m_demuxer->getFrame(frame);
    EXPECT_EQ(res, HJ_OK);
    ASSERT_NE(frame, nullptr);
    
    int64_t pts = frame->getPTS();
    EXPECT_LT(pts, 500) << "After reset, should start from beginning";
    HJFLogi("After reset, first frame pts: {}ms", pts);
}

//TEST_F(HJComplexDemuxerTest, Reset_AfterSeek_BackToStart) {
//    auto url = createM3U8Url(kSampleM3U8);
//    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
//    
//    int64_t duration = m_demuxer->getDuration();
//    m_demuxer->seek(duration / 2);
//    
//    // reset
//    m_demuxer->reset();
//    
//    // 读取第一帧
//    HJMediaFrame::Ptr frame = nullptr;
//    int res = m_demuxer->getFrame(frame);
//    EXPECT_EQ(res, HJ_OK);
//    ASSERT_NE(frame, nullptr);
//    
//    int64_t pts = frame->getPTS();
//    EXPECT_LT(pts, 500) << "After reset, should start from beginning";
//}

//***********************************************************************************//
// 五、Duration测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, GetDuration_BeforeInit_ReturnsZero) {
    int64_t duration = m_demuxer->getDuration();
    EXPECT_EQ(duration, 0);
}

TEST_F(HJComplexDemuxerTest, GetDuration_SampleM3U8_ValidDuration) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    EXPECT_GT(duration, 0) << "Duration should be positive";
    HJFLogi("Sample M3U8 duration: {}ms ({:.2f}s)", duration, duration/1000.0);
}

TEST_F(HJComplexDemuxerTest, GetDuration_LongM3U8_LargeDuration) {
    auto url = createM3U8Url(kLongM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    EXPECT_GT(duration, 60000) << "Long M3U8 should have duration > 60s";
    HJFLogi("Long M3U8 duration: {}ms ({:.2f}min)", duration, duration/60000.0);
}

//***********************************************************************************//
// 六、Done测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Done_CleansResources) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // 读取一些帧
    readFrames(50);
    
    // done
    m_demuxer->done();
    
    // 尝试再次读取，应该失败
    HJMediaFrame::Ptr frame = nullptr;
    int res = m_demuxer->getFrame(frame);
    EXPECT_NE(res, HJ_OK);
}

TEST_F(HJComplexDemuxerTest, Done_MultipleCallsSafe) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    // 多次调用done不应该崩溃
    m_demuxer->done();
    m_demuxer->done();
    m_demuxer->done();
}

TEST_F(HJComplexDemuxerTest, Done_ThenReinit_Success) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    readFrames(50);
    m_demuxer->done();
    
    // 重新初始化
    int res = m_demuxer->init(url);
    EXPECT_EQ(res, HJ_OK);
    
    // 应该能正常读取
    HJMediaFrame::Ptr frame = nullptr;
    res = m_demuxer->getFrame(frame);
    EXPECT_EQ(res, HJ_OK);
    EXPECT_NE(frame, nullptr);
}

//***********************************************************************************//
// 七、Loop循环测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Loop_TwiceLoop_DurationDoubled) {
    auto url = createM3U8Url(kSampleM3U8, 2);  // loop=2
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    HJFLogi("M3U8 with loop=2, duration: {}ms", duration);
    
    // 创建另一个不循环的demuxer对比
    auto demuxer2 = std::make_shared<HJComplexDemuxer>();
    auto url2 = createM3U8Url(kSampleM3U8, 1);
    ASSERT_EQ(demuxer2->init(url2), HJ_OK);
    int64_t singleDuration = demuxer2->getDuration();
    
    // loop=2的duration应该约等于loop=1的2倍
    EXPECT_GE(duration, singleDuration * 2 - 1000);  // 允许1秒误差
    EXPECT_LE(duration, singleDuration * 2 + 1000);
    
    HJFLogi("Single duration: {}ms, Loop2 duration: {}ms", singleDuration, duration);
    demuxer2->done();
}

//***********************************************************************************//
// 八、MediaInfo测试
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, MediaInfo_ContainsValidInfo) {
    auto url = createM3U8Url(kSampleM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    auto& mediaInfo = m_demuxer->getMediaInfo();
    ASSERT_NE(mediaInfo, nullptr);
    
    int64_t duration = mediaInfo->getDuration();
    EXPECT_GT(duration, 0);
    
    HJFLogi("MediaInfo duration: {}ms", duration);
}

//***********************************************************************************//
// 九、压力测试（可选）
//***********************************************************************************//

TEST_F(HJComplexDemuxerTest, Stress_MultipleSeekRead) {
    auto url = createM3U8Url(kDiscontinuityM3U8);
    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
    
    int64_t duration = m_demuxer->getDuration();
    
    // 多次seek和read
    for (int i = 0; i < 10; ++i) {
        int64_t seekPos = (duration * i) / 10;
        int res = m_demuxer->seek(seekPos);
        EXPECT_EQ(res, HJ_OK) << "Seek iteration " << i << " failed";
        
        int frames = readFrames(10);
        EXPECT_GT(frames, 0) << "Should read some frames after seek";
    }
}

//TEST_F(HJComplexDemuxerTest, Stress_ResetLoop) {
//    auto url = createM3U8Url(kSampleM3U8);
//    ASSERT_EQ(m_demuxer->init(url), HJ_OK);
//    
//    // 多次reset和read
//    for (int i = 0; i < 5; ++i) {
//        readFrames(50);
//        m_demuxer->reset();
//        
//        HJMediaFrame::Ptr frame = nullptr;
//        int res = m_demuxer->getFrame(frame);
//        EXPECT_EQ(res, HJ_OK);
//        EXPECT_NE(frame, nullptr);
//        
//        int64_t pts = frame->getPTS();
//        EXPECT_LT(pts, 500) << "After reset iteration " << i << ", should be at start";
//    }
//}

#endif //#if HJ_DEMUXER_TEST