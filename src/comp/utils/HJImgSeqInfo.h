#pragma once

#include "HJPrerequisites.h"
#include "HJReflCppJson.h"
#include "HJComUtils.h"
#include "HJNotify.h"
#include "HJMediaData.h"
#include "HJThreadPool.h"
#include "HJTransferInfo.h"
#include "HJAsyncCache.h"

#if defined(HarmonyOS)
    #include <GLES3/gl3.h>
#else
    typedef unsigned int GLuint;
#endif
NS_HJ_BEGIN

class HJImgSeqConfigPosInfo : public HJReflCppJsonInterpreter<HJImgSeqConfigPosInfo>
{
 public:    
 	double topx = 0.0;
 	double topy = 0.0;
 	double width = 1.0;    
    double height = 1.0;  
};

class HJImgSeqConfigInfo : public HJReflCppJsonInterpreter<HJImgSeqConfigInfo>
{
public:
 	std::string prefix = "";
 	int loops = 1;
 	int fps = 30;
    
    HJImgSeqConfigPosInfo position;
};

class HJOGCopyShaderStrip;
class HJImgSeqParse : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJImgSeqParse);
    HJImgSeqParse() = default;
    virtual ~HJImgSeqParse();
    int init(HJBaseParam::Ptr i_param);

    int update();
    //int render(HJTransferRenderModeInfo::Ptr i_renderModeInfo, int i_targetWidth, int i_targetHeight);

    static std::vector<std::string> parseConfig(const std::string &i_path, HJImgSeqConfigInfo &o_configInfo);
    void setNotify(HJBaseNotify i_notify)
    {
        m_notify = i_notify;
    }
    const HJImgSeqConfigPosInfo& getPositionInfo() const
    {
        return m_configInfo.position;
    }
    int getWidth();
    int getHeight();
    GLuint getTextureId();
    bool IsStateReady();
private:
    int priCreateTexture();
    int priUpdate(HJRawImageDataInfo::Ptr i_rawInfo);
    void priDone();

    HJBaseNotify m_notify = nullptr;
    HJRenderIndexCb m_idxNotify = nullptr;
    HJImgSeqConfigInfo m_configInfo;
    HJTimerThreadPool::Ptr m_threadTimer = nullptr;
    std::vector<std::string> m_pngUrlQueue; 
    int m_pngIdx = 0;
    int m_pngLoopIdx = 0;
    bool m_bEnd = false;
    HJAsyncCache<HJRawImageDataInfo::Ptr> m_cache;
    
    bool m_bTextureReady = false;
    GLuint m_texture = 0;

    int m_width = 0;
    int m_height = 0;
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJImgSeqConfigPosInfo, topx, topy, width, height);
REFL_AUTO_SIMPLE(HJ::HJImgSeqConfigInfo, prefix, loops, fps, position)
