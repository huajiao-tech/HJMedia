#include "HJEntryBaseRender.h"
#include "HJFLog.h"

#include "HJRteGraphProc.h"

#include "HJComEvent.h"
#include "HJGPUToRAM.h"
#include "HJMediaInfo.h"
#include "HJOGEGLSurface.h"
#include "HJRteComDraw.h"
#include "HJOGBaseShader.h"
#include "HJRteComSource.h"
#include "HJPrioUtils.h"
#include "HJRteGraphSetupInfo.h"
#include "HJFacePointsReal.h"

NS_HJ_BEGIN

//int HJEntryBaseRender::s_blurCascadeNum = 5;

HJEntryBaseRender::HJEntryBaseRender()
{

}
HJEntryBaseRender::~HJEntryBaseRender()
{

}

// int HJEntryBaseRender::contextInit(const HJEntryContextInfo &i_contextInfo)
// {
//     int i_err = 0;
//     do
//     {
//         i_err = HJ::HJEntryContext::init(HJEntryType_Pusher, i_contextInfo);
//         if (i_err < 0)
//         {
//             break;
//         }    
//     } while (false);
//     return i_err;
// }

void HJEntryBaseRender::doneRender()
{
    if (m_graphProc)
    {
        m_graphProc->done();
        m_graphProc = nullptr;
    }    
}

int HJEntryBaseRender::initRender(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        int graphProcType = HJRteGraphProcType_PLACEHOLDER_Default;
        HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJRteGraphProcType), graphProcType);
        m_graphProc = HJRteGraphProc::createRteGraphProc((HJRteGraphProcType)graphProcType);
        if (!m_graphProc)
        {
            i_err = -1;
            HJFLoge("{} HJGraphComVideoCapture create error", getInsName());
            break;
        }

        int insIdx = 0;
        HJ_CatchMapPlainGetVal(i_param, int, "InsIdx", insIdx);
        m_graphProc->setDebugIdx(insIdx);

        HJListener renderListener = [&](const HJNotification::Ptr ntf) -> int
        {
            if (!ntf)
            {
                return HJ_OK;
            }
            HJFLogi("{} graph video capture notify id:{}, msg:{}", getInsName(), HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID()), ntf->getMsg());
            std::string msg = ntf->getMsg();
            if (msg.empty())
            {
                msg = HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID());
            }
            int type = HJEntryContext::renderNotifyIdMap(ntf->getID());
            if (m_notify && (HJVIDEORENDERGRAPH_EVENT_NONE != type))
            {
                m_notify(type, msg);
            }
            return HJ_OK;
        };
        (*i_param)["renderListener"] = (HJListener)renderListener;

        i_err = m_graphProc->init(i_param);
        if (i_err < 0)
        {
            HJLoge("{} m_graphVideoCapture init error:{}",getInsName(),  i_err);
            break;
        }
    } while (false);
    return i_err;
}

int HJEntryBaseRender::nodeEnable(const std::string& i_classStyle, const std::string& i_insName, bool i_enable, const std::string& i_info)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeEnable(i_classStyle, i_insName, i_enable, i_info);
            if (i_err < 0)
            {
                HJFLoge("{} nodeEnable error i_err:{}", getInsName(), i_err);
                break;
            }
        }
    } while (false);
    return i_err;
}

int HJEntryBaseRender::nodeSetParam(const std::string& i_classStyle, const std::string& i_insName,
    const std::shared_ptr<HJBaseParam>& i_param)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeSetParam(i_classStyle, i_insName, i_param);
            if (i_err < 0)
            {
                HJFLoge("{} nodeSetParam error i_err:{}",getInsName(),  i_err);
                break;
            }
        }
    } while (false);
    return i_err;
}

int HJEntryBaseRender::nodeCreate(const std::string& i_nodeInfo)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeCreate(i_nodeInfo);
            if (i_err < 0)
            {
                HJFLoge("{} nodeCreate error i_err:{}",getInsName(),  i_err);
                break;
            }
        }
    } while (false);
    return i_err;   
}
int HJEntryBaseRender::nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, 
                         const std::string& i_dstClassStyle, const std::string& i_dstInsName, 
                         const std::string &i_shaderStyle, const std::string& i_linkInfo)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeConnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_shaderStyle, i_linkInfo);
            if (i_err < 0)
            {
                HJFLoge("{} nodeConnect error i_err:{}", getInsName(), i_err);
                break;
            }
        }
    } while (false);
    return i_err;      
}
int HJEntryBaseRender::nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeDelete(i_classStyle, i_insName, i_relink);
            if (i_err < 0)
            {
                HJFLoge("{} nodeDelete error i_err:{}",getInsName(),  i_err);
                break;
            }
        }
    } while (false);
    return i_err;      
}

int HJEntryBaseRender::nodeDisconnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
                            const std::string& i_dstClassStyle, const std::string& i_dstInsName,
                            const std::string& i_linkId)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeDisconnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkId);
            if (i_err < 0)
            {
                HJFLoge("{} nodeDisConnect error i_err:{}", getInsName(), i_err);
                break;
            }
        }
    } while (false);
    return i_err;     
}
int HJEntryBaseRender::nodeLinkInfoChange(const std::string& i_srcClassStyle, const std::string& i_srcInsName,
     const std::string& i_dstClassStyle, const std::string& i_dstInsName,
     const std::string& i_linkInfo)
{
    int i_err = 0;
    do
    {
        if (m_graphProc)
        {
            i_err = m_graphProc->nodeLinkInfoChange(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_linkInfo);
            if (i_err < 0)
            {
                HJFLoge("{} nodelinkinfochange error i_err:{}", getInsName(), i_err);
                break;
            }
        }
    } while (false);
    return i_err;     
}
std::string HJEntryBaseRender::nodeGetPre(const std::string& i_curClassStyle, const std::string& i_curInsName)
{
    if (!m_graphProc)
    {
        return "";
    }
    return m_graphProc->nodeGetPre(i_curClassStyle, i_curInsName);
}
std::string HJEntryBaseRender::nodeGetNext(const std::string& i_curClassStyle, const std::string& i_curInsName)
{
    if (!m_graphProc)
    {
        return "";
    }
    return m_graphProc->nodeGetNext(i_curClassStyle, i_curInsName);
}

int HJEntryBaseRender::setBaseNativeWindow(const std::string &i_classStyle, const std::string &i_insName, void* window, int i_width, int i_height, int i_state, int i_fps)
{
    int i_err = HJ_OK;
    do 
    {
        HJFLogi("{} setRenderNativeWindow enter w:{} h:{} state:{}", getInsName(), i_width, i_height, i_state);
        HJTransferRenderTargetInfo renderTargetInfo;
        renderTargetInfo.nativeWindow = (int64_t)window;
        renderTargetInfo.width = i_width;
        renderTargetInfo.height = i_height;
        renderTargetInfo.state = i_state;
        renderTargetInfo.fps = i_fps;
        std::string renderTargetStr = renderTargetInfo.serial();
        if (renderTargetStr.empty())
        {
            HJFLoge("initSerial error");
            i_err = -1;
            break;
        }    
        if (m_graphProc)
        {
            std::shared_ptr<HJOGEGLSurface> o_eglSurface = nullptr;
            HJFLogi("{} eglSurfaceProc enter w:{} h:{} state:{}", getInsName(), i_width, i_height, i_state);
            i_err = m_graphProc->eglSurfaceProc(renderTargetStr, o_eglSurface);
            if (i_err < 0)
            {
                HJFLoge("{} eglSurfaceProc error i_err:{} w:{} h:{} state:{}", getInsName(), i_err, i_width, i_height, i_state);
                break;
            }
            HJFLogi("{} procWindow enter w:{} h:{} state:{}",getInsName(),  i_width, i_height, i_state);
            i_err = m_graphProc->procWindow(i_classStyle, i_insName, o_eglSurface, i_state);
            if (i_err < 0)
            {
                HJFLoge("{} priProceRteGraph error i_err:{} w:{} h:{} state:{}", getInsName(), i_err, i_width, i_height, i_state);
                break;
            }
        }
        else
        {
            i_err = -1;
            HJFLoge("{} m_graphProc is null setBaseNativeWindow is invalid", getInsName());
            break;
        }
    } while (false);
    HJFLogi("{} setRenderNativeWindow end w:{} h:{} state:{} i_err:{}", getInsName(), i_width, i_height, i_state, i_err);
    return i_err;
}
// int HJEntryBaseRender::setRenderNativeWindow(void *window, int i_width, int i_height, int i_state, int i_type, int i_fps, std::shared_ptr<HJOGEGLSurface> & o_eglSurface)
// {
//     int i_err = 0;
//     do
//     {
//         HJFLogi("setRenderNativeWindow enter w:{} h:{} state:{}", i_width, i_height, i_state);
//         HJTransferRenderTargetInfo renderTargetInfo;
//         renderTargetInfo.nativeWindow = (int64_t)window;
//         renderTargetInfo.width = i_width;
//         renderTargetInfo.height = i_height;
//         renderTargetInfo.state = i_state;
//         renderTargetInfo.type = i_type;
//         renderTargetInfo.fps = i_fps;
//         std::string renderTargetStr = renderTargetInfo.initSerial();
//         if (renderTargetStr.empty())
//         {
//             HJFLoge("initSerial error");
//             i_err = -1;
//             break;
//         }    
//         if (m_graphProc)
//         {
//             i_err = m_graphProc->eglSurfaceProc(renderTargetStr, o_eglSurface);
//             if (i_err < 0)
//             {
//                 HJFLoge("eglSurfaceProc error i_err:{} w:{} h:{} state:{}", i_err, i_width, i_height, i_state);
//                 break;
//             }
//             i_err = m_graphProc->procEGLSurface(o_eglSurface, i_type, i_state);
//             if (i_err < 0)
//             {
//                 HJFLoge("priProceRteGraph error i_err:{} w:{} h:{} state:{}", i_err, i_width, i_height, i_state);
//                 break;
//             }
//         }
//     } while (false);
//     return i_err;
// }

int HJEntryBaseRender::openNativeSource(bool i_bUsePBO)
{
    int i_err = HJ_OK;
    HJFLogi("{} openNativeSource enter bUsePBO:{}", getInsName(), i_bUsePBO);
    //HJ_LOCK(m_nativeSourceLock);
    do 
    {
        if (!m_graphProc)
        {
            i_err = -1;
            break;
        }
        if (!m_gpuToRamPtr)
        {
            HJGPUToRAMType type = i_bUsePBO ? HJGPUToRAMType_PBO : HJGPUToRAMType_ImageReceiver;
            m_gpuToRamPtr = HJBaseGPUToRAM::CreateGPUToRAM(type);

            HJBaseParam::Ptr param = HJBaseParam::Create();
            if (!i_bUsePBO)
            {
                HJGPUToRAMImageReceiver::HOImageReceiverSurfaceCb surfaceCb = [this](void *i_window, int i_width, int i_height, bool i_bCreate)
                {
                    int state = 0;
                    if (i_bCreate)
                    {
                        state = HJTargetState_Create;
                    }
                    else
                    {
                        state = HJTargetState_Destroy;
                    }
                    return setBaseNativeWindow(HJRteGraphConfig::HJNodeClass_TargetImgReceiver,HJRteGraphConfig::HJNodeClass_TargetImgReceiver, i_window, i_width, i_height, state,  30);
                };
                HJ_CatchMapSetVal(param, HJGPUToRAMImageReceiver::HOImageReceiverSurfaceCb, surfaceCb);
            }

            i_err = m_gpuToRamPtr->init(param);
            if (i_err < 0)
            {
                break;
            }    
    
            if (i_bUsePBO)
            {
                HJBaseGPUToRAM::Wtr wtr = m_gpuToRamPtr;
                m_graphProc->openPBO([wtr](HJSPBuffer::Ptr i_buffer, int width, int height)
                {
                    HJBaseGPUToRAM::Ptr gpuToRamPtr = wtr.lock();
                    if (gpuToRamPtr)
                    {
                        gpuToRamPtr->setMediaData(i_buffer, width, height);
                    }
                    return HJ_OK;
                });
            }
                           
            m_graphProc->openFaceEffect();
            //m_bNativeSourceOpen = true;
        }    
    } while (false);
    HJFLogi("{} openNativeSource end bUsePBO:{} i_err:{}", getInsName(), i_bUsePBO, i_err);
    return i_err;
}
void HJEntryBaseRender::closeNativeSource()
{
    HJFLogi("{} closeNativeSource enter", getInsName());

    //HJ_LOCK(m_nativeSourceLock);
    if (m_graphProc)
    {
        m_graphProc->closePBO();
        m_graphProc->closeFaceEffect();
    }
    
    if (m_gpuToRamPtr)
    {
        m_gpuToRamPtr->done();
        m_gpuToRamPtr = nullptr;
    }   

    //m_bNativeSourceOpen = false;

    HJFLogi("{} closeNativeSource end", getInsName());
}
std::shared_ptr<HJRGBAMediaData> HJEntryBaseRender::acquireNativeSource()
{
    HJRGBAMediaData::Ptr data = nullptr;
    if (m_gpuToRamPtr)
    {   
        data = m_gpuToRamPtr->getMediaRGBAData();
        //HJFLogi("acquireNativeSource enter:{}", data ? "has data" : "no data");
    }    
    return data;
}
void HJEntryBaseRender::setFaceInfo(const std::string& i_sourceInsName, int i_width, int i_height, const std::string& i_faceInfo, bool i_bDebugPoint)
{
    if (m_graphProc)
    {
        //HJFLogi("setFaceInfo enter:{} w:{} h:{}", faceInfo->m_faceInfo, faceInfo->m_width, faceInfo->m_height);
        m_graphProc->setFaceInfo(i_sourceInsName, i_faceInfo);
        m_graphProc->setFaceInfo(i_sourceInsName, i_width, i_height, i_faceInfo, i_bDebugPoint);
    }  
}
void HJEntryBaseRender::setVideoEncQuantOffset(int i_quantOffset)
{
    m_quantOffset = i_quantOffset;
}

void HJEntryBaseRender::manualDrive()
{
    if (m_graphProc)
    {
        m_graphProc->manualDrive();
    }
}
void HJEntryBaseRender::prepareROI(const std::string& i_sourceInsName, const HJKeyStorage::Ptr& i_param, int i_encWidth, int i_encHeight)
{
    HJRoiEncodeCb roicb = [this, i_sourceInsName, i_encWidth, i_encHeight](HJRoiInfoVectorPtr &roiInfo, int i_nFlag)
    {
        if (i_nFlag == HJ_ROI_CB_FLAG_TRY_ACQUIRE)
        {
            if (m_graphProc)
            {
                std::string faceInfo = i_sourceInsName.empty() ? "" : m_graphProc->getFaceInfo(i_sourceInsName);

                std::shared_ptr<HJMoreFacePointsReal> pts = nullptr;
                if (!faceInfo.empty())
                {
                    pts = HJMoreFacePointsReal::Create();
                    if (pts->deserial(faceInfo) == HJ_OK)
                    {
                        int64_t systeTime = pts->getSystemTime();
                        int64_t timeElapse = abs(HJCurrentSteadyMS() - pts->getSystemTime());
                        if (timeElapse > 1000)
                        {
                            HJFLogi("faceoutput the elapse is:{}, systemTime:{} erase the cacheFaceInfo", timeElapse, pts->getSystemTime());
                        }
                        else
                        {
                            int imgw = pts->width();
                            int imgh = pts->height();
                            std::vector<HJSingleFacePointsReal::Ptr> &singlePt = pts->getPoints();
                            for (auto &single : singlePt)
                            {
                                HJRoiInfo roi;
                                roi.x = single->getFaceRect().x * i_encWidth / imgw;
                                roi.y = single->getFaceRect().y * i_encHeight / imgh;
                                roi.w = single->getFaceRect().w * i_encWidth / imgw;
                                roi.h = single->getFaceRect().h * i_encHeight / imgh;
                                roi.quant_offset = m_quantOffset;
                                roiInfo = std::make_shared<HJRoiInfoVector>();
                                roiInfo->push_back(std::move(roi));
                                HJFLogi("{} roi cb enter flag:{} find face x,y:{} {} w,h:{} {} quant:{}", getInsName(), i_nFlag, roi.x, roi.y, roi.w, roi.h, roi.quant_offset);
                            }
                        }
                    }
                }
            }
        }
        // if (i_nFlag == HJ_ROI_CB_FLAG_TRY_ACQUIRE)
        // {
        //     {
        //         HJ_LOCK(m_nativeSourceLock);
        //         if (!m_bNativeSourceOpen)
        //         {
        //             m_cache.clear();
        //             m_pointSubscriberPtr = nullptr;
        //             HJFLogi(" roi cb enter flag:{} nativesource open false destroy roicom", i_nFlag);
        //         }
        //     }
            
        //     if (!m_pointSubscriberPtr)
        //     {
        //         HJ_LOCK(m_nativeSourceLock);
        //         if (m_bNativeSourceOpen)
        //         {
        //             m_pointSubscriberPtr = std::make_shared<HJFaceSubscribeFunc>([this](HJFacePointsReal::Ptr i_point)
        //             {
        //                 m_cache.enqueue(i_point);
        //             });
        //             m_graphProc->registSubscriber(m_pointSubscriberPtr);
                    
        //             HJFLogi(" roi cb enter flag:{} nativesource open true create roicom", i_nFlag);
        //         }
        //     }
        //     if (m_pointSubscriberPtr)
        //     {
        //         HJFacePointsReal::Ptr point = m_cache.acquire();     
        //         if (point)
        //         {
        //             if (point->isContainFace())
        //             {
        //                 const std::vector<HJPointf> &filterPt = point->getFilterPt();
        //                 int imgw = point->width();
        //                 int imgh = point->height();

        //                 HJRoiInfo roi;
        //                 roi.x = filterPt[5].x * i_encWidth / imgw;
        //                 roi.y = filterPt[5].y * i_encHeight / imgh;
        //                 roi.w = (filterPt[8].x - filterPt[5].x) * i_encWidth / imgw;
        //                 roi.h = (filterPt[8].y - filterPt[5].y) * i_encHeight / imgh;
        //                 roi.quant_offset = m_quantOffset;
        //                 roiInfo = std::make_shared<HJRoiInfoVector>();
        //                 roiInfo->push_back(std::move(roi));
        //                 HJFLogi(" roi cb enter flag:{} find face x,y:{} {} w,h:{} {} quant:{}", i_nFlag, roi.x, roi.y, roi.w, roi.h, roi.quant_offset);
        //             }    
        //             else
        //             {
        //                 HJFLogi(" roi cb enter flag:{} not find face", i_nFlag);
        //             }    
        //             m_cache.recovery(point);
        //         }             
        //     }   
        // }   
        // else if (i_nFlag == HJ_ROI_CB_FLAG_CLOSE)
        // {
        //     m_cache.clear();
        //     m_pointSubscriberPtr = nullptr;
        //     HJFLogi(" roi cb enter flag:{} destroy roicom", i_nFlag);
        // }        
    };
    (*i_param)["ROIEncodeCb"] = roicb;     
}
int HJEntryBaseRender::openFaceu(const std::string &i_faceuUrl)
{
    int i_err = HJ_OK;
    do 
    {
        HJFLogi("{} openFaceu enter url:{}", getInsName(), i_faceuUrl);
        if (m_graphProc)
        {
            // HJBaseParam::Ptr param = HJBaseParam::Create();
            // HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, i_faceuUrl);
            // HJ_CatchMapPlainSetVal(param, bool, "bDebugPoint", i_bDebugPoint);

            i_err = m_graphProc->nodeEnable(HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, true, i_faceuUrl);
            // i_err = m_graphProc->openFaceu(param);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    HJFLogi("{} openFaceu end i_err:{}", getInsName(),  i_err);
    return i_err;
}
void HJEntryBaseRender::closeFaceu()
{
    if (m_graphProc)
    {   
        m_graphProc->nodeEnable(HJRteGraphConfig::HJNodeClass_SourceFaceu, HJRteGraphConfig::HJNodeClass_SourceFaceu, false, "");
    } 
}
void HJEntryBaseRender::openEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    int effctType = HJRteEffect_UNKNOWN;
    if (i_effectType == HJPrioEffect_Gray)
    {
        effctType = HJRteEffect_Gray;
    }
    else if (i_effectType == HJPrioEffect_Blur)
    {
        effctType = HJRteEffect_Blur;
    }
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), effctType);
    if (m_graphProc)
    {
        HJFLogi("{} HJNAPIPlayer openeffect enter", getInsName());
        m_graphProc->openEffect(param);
    }
}
void HJEntryBaseRender::closeEffect(int i_effectType)
{
    HJBaseParam::Ptr param = HJBaseParam::Create();
    int effctType = HJRteEffect_UNKNOWN;
    if (i_effectType == HJPrioEffect_Gray)
    {
        effctType = HJRteEffect_Gray;
    }
    else if (i_effectType == HJPrioEffect_Blur)
    {
        effctType = HJRteEffect_Blur;
    }
    HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), effctType);
    if (m_graphProc)
    {
        m_graphProc->closeEffect(param);
    } 
}
void HJEntryBaseRender::setFaceProtected(bool i_bProtect)
{
    HJFLogi("{} setFaceProtected enter i_bProtect:{}",getInsName(),  i_bProtect);
    if (m_graphProc)
    {
        m_graphProc->setFaceProtected(i_bProtect);
    }
    HJFLogi("setFaceProtected end i_bProtect:{}", i_bProtect);
}

int HJEntryBaseRender::openPngSeq(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
       if (m_graphProc)
       {
           i_err = m_graphProc->openPNGSeq(i_param);
           if (i_err < 0)
           {
               break;
           }
       }
    } while (false);
    return i_err;
}


// int HJEntryBaseRender::priProcRteCreateUI(const std::shared_ptr<HJOGEGLSurface> &i_eglSurface)
// {
//     int i_err = 0;
//     HJFLogi("HJEntryBaseRender::priProcRteCreateUI enter");
//     do
//     {
// #if USE_RTE
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         std::weak_ptr<HJOGEGLSurface> weglSurface = i_eglSurface;
//         m_graphProc->graphProcAsync([this, wgraphProc, weglSurface]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             HJOGEGLSurface::Ptr eglSurface = weglSurface.lock();
//             if (!graphProc || !eglSurface)
//             {
//                 return HJ_OK;
//             }

//             HJFLogi("HJEntryBaseRender::priProcRteCreateUI keranl thread graph proc");

//             HJRteComDrawEGLUI_0::Ptr uicom = HJRteComDrawEGLUI_0::Create();
//             uicom->setInsName("uicom");
//             // uicom->setPriority(HJRteComPriority_Target);
//             uicom->setSurface(eglSurface);
//             uicom->setFps(eglSurface->getFps());
//             graphProc->addTarget(uicom);

//             if (graphProc->getSourceType() == HJRteComSourceType_SPLITSCREEN) // single screen;
//             {
//                 HJOGBaseShader::Ptr shaderSplitScreenLROES = HJOGBaseShader::createShader(HJOGBaseShaderType_SplitScreenLR_OES);
//                 HJOGCopyShaderStrip::Ptr copyShader = std::dynamic_pointer_cast<HJOGCopyShaderStrip>(shaderSplitScreenLROES);
//                 if (copyShader)
//                 {
//                     copyShader->setForceXMirror(graphProc->isSplitScreenMirror());
//                 }
//                 graphProc->connectCom(*graphProc->getSources().begin(), uicom, shaderSplitScreenLROES);
//             }
//             else if (graphProc->getSourceType() == HJRteComSourceType_PLACEHOLDER_Default)
//             {
//                 HJRteComDrawCopyOESFBO::Ptr video2D = HJRteComDrawCopyOESFBO::Create();
//                 video2D->init(nullptr);
//                 graphProc->addFilter(video2D);
//                 graphProc->connectCom(*graphProc->getSources().begin(), video2D);

//                 // HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
//                 //  graphProc->connectCom(video2D, uicom, shader2D);

//                 HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
//                 HJRteComDrawPBOFBO::Ptr pbo = HJRteComDrawPBOFBO::Create();
//                 // pbo->setReadCb([](HJSPBuffer::Ptr i_buffer, int width, int height)
//                 // {
//                 //     HJFLogi("pbo readCb width:{}, height:{}", width, height);
//                 //     return HJ_OK; 
//                 // });
//                 pbo->setEnable(false);
//                 pbo->init(nullptr);
//                 pbo->setFps(eglSurface->getFps());
//                 graphProc->addTarget(pbo); // target is fbo
//                 graphProc->connectCom(video2D, pbo, shader2D);

//                 HJRteComDrawBlurCascadeFBO::Ptr blur = HJRteComDrawBlurCascadeFBO::Create();
//                 blur->setCascadeNum(s_blurCascadeNum);
//                 blur->init(nullptr);
//                 blur->setEnable(false);
//                 graphProc->addFilter(blur);

//                 graphProc->connectCom(video2D, blur);
//                 graphProc->connectCom(blur, uicom, shader2D);

//                 // faceu
//                 HJRteComSourceFaceu::Ptr faceu = HJRteComSourceFaceu::Create();
//                 faceu->init(nullptr);
//                 faceu->setEnable(false);

//                 graphProc->addSource(faceu);
//                 HJOGBaseShader::Ptr shaderNoPreMul2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
//                 graphProc->connectCom(faceu, uicom, shaderNoPreMul2D);

//                 HJFLogi("HJEntryBaseRender::priProcRteCreateUI keranl thread graph proc end");

//             }
//             return HJ_OK;
//         });
// #endif
//     } while (false);
//     HJFLogi("HJEntryBaseRender::priProcRteCreateUI end i_err:{}", i_err);
//     return i_err;
// }
// void HJEntryBaseRender::priProcRteDestroyUI()
// {
// #if USE_RTE    
//     do
//     {
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         m_graphProc->graphProcAsync([this, wgraphProc]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             if (!graphProc)
//             {
//                 return HJ_OK;
//             }
//             graphProc->foreachUseType<HJRteComDrawEGLUI_0>([graphProc](HJRteCom::Ptr i_com)
//             {
//                 graphProc->removeCom(i_com);
//                 return HJ_OK;
//             });
//             return HJ_OK;
//         });
//     } while (false);
// #endif    
// }

// int HJEntryBaseRender::priProcRteCreateEncoder(const std::shared_ptr<HJOGEGLSurface> &i_eglSurface)
// {
//     int i_err = 0;
// #if USE_RTE    
//     do
//     {
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         std::weak_ptr<HJOGEGLSurface> weglSurface = i_eglSurface;
//         m_graphProc->graphProcAsync([this, wgraphProc, weglSurface]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             HJOGEGLSurface::Ptr eglSurface = weglSurface.lock();
//             if (!graphProc || !eglSurface)
//             {
//                 return HJ_OK;
//             }

//             HJFLogi("HJEntryBaseRender::priProcRteCreateUI keranl thread graph proc");

//             HJRteComDrawEGLEncoder::Ptr encoderTarget = HJRteComDrawEGLEncoder::Create();
//             encoderTarget->setInsName("encoderTarget");
//             encoderTarget->setSurface(eglSurface);
//             encoderTarget->setFps(eglSurface->getFps());
//             graphProc->addTarget(encoderTarget);
//             graphProc->foreachUseType<HJRteComDrawBlurCascadeFBO>([graphProc, encoderTarget](HJRteCom::Ptr i_blur) 
// 	        { 
//                 HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
// 		        graphProc->connectCom(i_blur, encoderTarget, shader2D);
// 		        return HJ_OK;
// 	        });

//             graphProc->foreachUseType<HJRteComSourceFaceu>([graphProc, encoderTarget](HJRteCom::Ptr i_com) 
// 	        { 
//                 HJOGBaseShader::Ptr shaderNoPreMul2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
//                 graphProc->connectCom(i_com, encoderTarget, shaderNoPreMul2D);

// 		        return HJ_OK;
// 	        });


//             HJRteComSourceImgSeq::Ptr pngSeq = graphProc->getUseTypeCom<HJRteComSourceImgSeq>();
//             if (pngSeq)
//             {
//                 HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
//                 HJRectf rect = pngSeq->getRect();
//                 graphProc->connectCom(pngSeq, encoderTarget, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(rect.x, rect.y, rect.w, rect.h, HJWindowRenderMode_CLIP, false, true));
//             }
            
//             return HJ_OK;
//         });
//     } while (false);
// #endif    
//     return i_err;
// }
// void HJEntryBaseRender::priProcRteDestroyEncoder()
// {
// #if USE_RTE    
//     do
//     {
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         m_graphProc->graphProcAsync([this, wgraphProc]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             if (!graphProc)
//             {
//                 return HJ_OK;
//             }
//             graphProc->foreachUseType<HJRteComDrawEGLEncoder>([graphProc](HJRteCom::Ptr i_com)
//             {            
//                 graphProc->removeCom(i_com);
//                 return HJ_OK;
//             });
//             return HJ_OK;
//         });
//     } while (false);
// #endif    
// }
// int HJEntryBaseRender::priProcRteCreateImgReceiver(const std::shared_ptr<HJOGEGLSurface> &i_eglSurface)
// {
//     int i_err = 0;
// #if USE_RTE    
//     do
//     {
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         std::weak_ptr<HJOGEGLSurface> weglSurface = i_eglSurface;
//         m_graphProc->graphProcAsync([this, wgraphProc, weglSurface]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             HJOGEGLSurface::Ptr eglSurface = weglSurface.lock();
//             if (!graphProc || !eglSurface)
//             {
//                 return HJ_OK;
//             }

//             HJRteComDrawEGLImgReceiver::Ptr imgReceiverTarget = HJRteComDrawEGLImgReceiver::Create();
//             imgReceiverTarget->setInsName("imgReceiverTarget");
//             imgReceiverTarget->setSurface(eglSurface);
//             imgReceiverTarget->setFps(eglSurface->getFps());
//             graphProc->addTarget(imgReceiverTarget);
//             HJOGBaseShader::Ptr shaderOES = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_OES);
//             graphProc->connectCom(*graphProc->getSources().begin(), imgReceiverTarget,  shaderOES);
//             return HJ_OK;
//         });
//     } while (false);
// #endif
//     return i_err;
// }
// void HJEntryBaseRender::priProcRteDestroyImgReceiver()
// {
// #if USE_RTE
//     do
//     {
//         std::weak_ptr<HJRteGraphProc> wgraphProc = m_graphProc;
//         m_graphProc->graphProcAsync([this, wgraphProc]()
//         {
//             HJRteGraphProc::Ptr graphProc = wgraphProc.lock();
//             if (!graphProc)
//             {
//                 return HJ_OK;
//             }
//             graphProc->foreachUseType<HJRteComDrawEGLImgReceiver>([graphProc](HJRteCom::Ptr i_com)
//             {
//                 graphProc->removeCom(i_com);
//                 return HJ_OK;
//             });
//             return HJ_OK;
//         });
//     } while (false);
// #endif    
// }

// int HJEntryBaseRender::priProcRteGraph(const std::shared_ptr<HJOGEGLSurface>& i_eglSurface, int i_type, int i_state)
// {
//     int i_err = 0;
//     do
//     {
//         if (i_state == HJTargetState_Create)
//         {
//             if (i_type == HJOGEGLSurfaceType_UI)
//             {
//                 i_err = priProcRteCreateUI(i_eglSurface);       
//             }
//             else if (i_type == HJOGEGLSurfaceType_EncoderPusher)
//             {
//                 i_err = priProcRteCreateEncoder(i_eglSurface);
//             }
//             else if (i_type == HJOGEGLSurfaceType_ImageReceiver)
//             {
//                 i_err = priProcRteCreateImgReceiver(i_eglSurface);
//             }
//             else
//             {
//                 HJFLoge("unknown surface type");
//             }
//         }
//         else if (i_state == HJTargetState_Destroy)
//         {
//             // i_eglSurface is empty;
//             if (i_type == HJOGEGLSurfaceType_UI)
//             {
//                 priProcRteDestroyUI();
//             }
//             else if (i_type == HJOGEGLSurfaceType_EncoderPusher)
//             {
//                 priProcRteDestroyEncoder();
//             }
//             else if (i_type == HJOGEGLSurfaceType_ImageReceiver)
//             {
//                 priProcRteDestroyImgReceiver();
//             }
//             else
//             {
//                 HJFLoge("unknown surface type");
//             }
//         }
//     } while (false);
//     return i_err;
// }

NS_HJ_END
