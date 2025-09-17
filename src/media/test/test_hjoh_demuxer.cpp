//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJOHDemuxer.h"
#include "HJFLog.h"
#include <memory>

NS_HJ_BEGIN

// 简单的测试函数，验证HJOHDemuxer的基本功能
void testHJOHDemuxer()
{
    HJFLogi("Testing HJOHDemuxer basic functionality...");
    
    // 创建一个HJOHDemuxer实例
    auto demuxer = std::make_shared<HJOHDemuxer>();
    
    // 创建媒体URL
    auto mediaUrl = std::make_shared<HJMediaUrl>("test_file.mp4");
    mediaUrl->setEnableVideo(true);
    mediaUrl->setEnableAudio(true);
    
    // 测试初始化
    int result = demuxer->init(mediaUrl);
    if (result == HJ_OK) {
        HJFLogi("HJOHDemuxer init success");
        
        // 测试获取媒体信息
        auto mediaInfo = demuxer->getMediaInfo();
        if (mediaInfo) {
            HJFLogi("Media duration: {} ms", mediaInfo->getDuration());
            HJFLogi("Video track index: {}", mediaInfo->m_vstIdx);
            HJFLogi("Audio track index: {}", mediaInfo->m_astIdx);
        }
        
        // 测试获取帧
        HJMediaFrame::Ptr frame;
        result = demuxer->getFrame(frame);
        if (result == HJ_OK && frame) {
            HJFLogi("Successfully got frame: {}", frame->formatInfo());
        } else if (result == HJ_EOF) {
            HJFLogi("Reached end of file");
        } else {
            HJFLoge("Failed to get frame, error: {}", result);
        }
        
        // 测试seek功能
        result = demuxer->seek(1000); // seek到1秒位置
        if (result == HJ_OK) {
            HJFLogi("Seek to 1000ms success");
        } else {
            HJFLoge("Seek failed, error: {}", result);
        }
        
        // 清理
        demuxer->done();
        HJFLogi("HJOHDemuxer test completed");
    } else {
        HJFLoge("HJOHDemuxer init failed, error: {}", result);
    }
}

NS_HJ_END

// 如果需要独立测试，可以取消下面的注释
/*
int main()
{
    HJ::testHJOHDemuxer();
    return 0;
}
*/