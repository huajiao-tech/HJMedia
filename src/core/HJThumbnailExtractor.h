//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJImageWriter.h"
#include "HJEnvironment.h"
#include "HJFFDemuxer.h"
#include "HJBaseCodec.h"
#include "HJMediaFrame.h"
#include "HJImageWriter.h"
#include "HJExecutor.h"

#define HJ_THUM_STEP_MIN   1000    //ms

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJThumbnailNotify{
    HJThumbnailNotify_Init = 0,
    HJThumbnailNotify_Url,
    HJThumbnailNotify_Completed,
    HJThumbnailNotify_Error,
} HJThumbnailNotify;

class HJThumbnailInfo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJThumbnailInfo>;
    HJThumbnailInfo(const std::string& workDir, int width, int height, int count, int64_t step = 0, std::string format = "jpg", float quality = 0.8f);
    virtual ~HJThumbnailInfo();
    
    void setSeekStep(const int64_t seekStep) {
        m_seekStep = seekStep;
    }
public:
    std::string m_workDir = "";
    int         m_width = 180;
    int         m_height = 320;
    int         m_count = 10;
    int64_t     m_seekStep = 0;//HJ_THUM_STEP_MIN;  //ms
    std::string m_format = "jpg";
    float       m_quality = 0.8f;
};

//***********************************************************************************//
class HJThumbnailExtractor : public HJEnvironment
{
public:
    HJ_DECLARE_PUWTR(HJThumbnailExtractor);
    HJThumbnailExtractor();
    virtual ~HJThumbnailExtractor();
    
    int init(const std::string& url, const HJThumbnailInfo::Ptr& thumbInfo);
    int run();
    int asyncRun();
    
    HJMediaInfo::Ptr& getMediaInfo() {
        return m_minfo;
    }
private:
    int decodeVideoFrame(HJMediaFrame::Ptr& outFrame);
    int notify(const size_t identify, const std::string& msg = "");
private:
    HJMediaUrl::Ptr         m_mediaUrl = nullptr;
    HJThumbnailInfo::Ptr   m_thumbInfo = nullptr;
    HJFFDemuxer::Ptr        m_source = nullptr;
    HJMediaInfo::Ptr        m_minfo = nullptr;
    HJBaseCodec::Ptr       m_decoder = nullptr;
    HJExecutor::Ptr        m_excutor = nullptr;
    int64_t                 m_seekPos = 0;
    HJImageWriter::Ptr     m_imageWriter = nullptr;
    bool                    m_bQuit = false;
};

NS_HJ_END
