#pragma once

#include "HJPrerequisites.h"
#include "HJEntryContext.h"
#include "HJAny.h"
#include "HJComutils.h"

NS_HJ_BEGIN

class HJBaseGPUToRAM;
class HJRGBAMediaData;
class HJFacePointsWrapper;
class HJPrioGraphProc;
class HJRteGraphProc;
class HJOGEGLSurface;

class HJEntryBaseRender 
{
public:
    HJ_DEFINE_CREATE(HJEntryBaseRender);
    HJEntryBaseRender();
    virtual ~HJEntryBaseRender();
    
    // static int contextInit(const HJEntryContextInfo& i_contextInfo);

    int initRender(HJBaseParam::Ptr i_param);
    void doneRender();

    //int setRenderNativeWindow(void* window, int i_width, int i_height, int i_state, int i_type, int i_fps, std::shared_ptr<HJOGEGLSurface> & o_eglSurface);
    
    int setBaseNativeWindow(const std::string &i_classStyle, const std::string &i_insName, void* window, int i_width, int i_height, int i_state, int i_fps);
    

    void openEffect(int i_effectType);
    void closeEffect(int i_effectType);

    int openNativeSource(bool i_bUsePBO = true);
    void closeNativeSource();
    std::shared_ptr<HJRGBAMediaData> acquireNativeSource();
    void setFaceInfo(const std::string& i_sourceInsName, int i_width, int i_height, const std::string& i_faceInfo, bool i_bDebugPoint = false);
    
    void prepareROI(const std::string& i_sourceInsName, const HJKeyStorage::Ptr& i_param, int i_width, int i_height);   
    void setVideoEncQuantOffset(int i_quantOffset);

    int openFaceu(const std::string &i_faceuUrl);
    void closeFaceu();
    void setFaceProtected(bool i_bProtect);

    int openPngSeq(HJBaseParam::Ptr i_param);

    void setInsName(const std::string &i_name)
    {
        m_insName = i_name;
    }
    const std::string& getInsName() const
    {
        return m_insName;
    }

    void manualDrive();

    int nodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info);
    int nodeSetParam(const std::string& i_classStyle, const std::string& i_insName, const std::shared_ptr<HJBaseParam>& i_param);
    int nodeCreate(const std::string& i_nodeInfo);
    int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, 
                         const std::string& i_dstClassStyle, const std::string& i_dstInsName, 
                         const std::string &i_shaderStyle, const std::string& i_linkInfo);
    int nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink = false);

    int nodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
                            const std::string& i_dstClassStyle, const std::string& i_dstInsName,
                            const std::string& i_linkId = ""); 
    int nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
     const std::string& i_dstClassStyle, const std::string& i_dstInsName,
     const std::string& i_linkInfo);
    std::string nodeGetPre(const std::string& i_curClassStyle, const std::string& i_curInsName);
    std::string nodeGetNext(const std::string& i_curClassStyle, const std::string& i_curInsName);

protected:
    std::shared_ptr<HJRteGraphProc> m_graphProc = nullptr;
    HJNAPIEntryNotify m_notify = nullptr;
private:

    std::string m_insName = "";
    std::shared_ptr<HJBaseGPUToRAM> m_gpuToRamPtr = nullptr;

    // std::mutex m_nativeSourceLock;
    // bool m_bNativeSourceOpen = false;

    std::atomic<int> m_quantOffset{-3};

    //HJFaceSubscribeFuncPtr m_pointSubscriberPtr = nullptr;
    //HJAsyncCache<std::shared_ptr<HJFacePointsReal>> m_cache;

    //static int s_blurCascadeNum;
};

NS_HJ_END
