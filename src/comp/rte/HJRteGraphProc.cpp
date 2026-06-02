
#include "HJFLog.h"
#include "HJRteCom.h"
#include "HJRteGraphProc.h"
#include "HJRteComSource.h"
#include "HJRteComDraw.h"
#include "HJComEvent.h"
#include "HJOGBaseShader.h"
#include "HJOGEGLSurface.h"
#include "HJCoreVersion.h"
#include "HJFacePointsInfo.h"
#include "HJPointSmooth.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGRenderEnv.h"
#endif
#include "HJFacePointsReal.h"

NS_HJ_BEGIN

int HJRteGraphProc::s_blurCascadeNum = 5;

HJRteGraphProc::HJRteGraphProc()
{
    HJ_SetInsName(HJRteGraphProc);
    m_facePointSmooth = std::make_shared<HJMorePointSmooth>();
}
HJRteGraphProc::~HJRteGraphProc()
{
    HJFLogi("{} ~HJRteGraphProc", getDebugName());
}
HJRteGraphProc::Ptr HJRteGraphProc::createRteGraphProc(HJRteGraphProcType i_type)
{
    HJRteGraphProc::Ptr ret = nullptr;
    switch (i_type)
    {
    case HJRteGraphProcType_PLACEHOLDER_Default:
        ret = HJRteGraphProcPlaceHolderDefault::Create();
        break;
    case HJRteGraphProcType_SPLITSCREEN:
        ret = HJRteGraphProcSplictScreen::Create();
        break;
    case HJRteGraphProcType_Test:
        ret = HJRteGraphProcTest::Create();
        break;
    case HJRteGraphProcType_CONFIG_SETUP:
        ret = HJRteGraphProcConfigSetup::Create();
    default:
        break;
    }
    return ret;
}
int HJRteGraphProc::priPreCheckGraph()
{
    int i_err = HJ_OK;
    do
    {
        i_err = foreachFilter([this](std::shared_ptr<HJRteCom> i_com)
            {
                int ret = HJ_OK;
                do 
                {
                    if (!i_com->isEnable() && (i_com->getPreCount() > 1))
                    {
                        ret = HJErrInvalidParams;
                        HJFLoge("{} filter:{} is disable, but pre com is more than 1, not support", getDebugName(), i_com->getInsName());
                        break;
                    }                 
                } while (false);
                return ret; 
            });

        if (m_bMainSourceRenderForce && (priGetMainFilterSource() == nullptr))
        {
            i_err = HJErrInvalidParams;
            HJFLoge("{} priGetMainFilterSource nullptr", getDebugName());
            break;
        }
    } while (false);
    return i_err;
}
void HJRteGraphProc::priStatRefCnt()
{
    foreachAllCom([this](std::shared_ptr<HJRteCom> i_com)
                  {
            i_com->refCntClear();
            return HJ_OK; });

    foreachSource([this](std::shared_ptr<HJRteCom> i_com)
                  {
            statRefCntFromTopToBottom(i_com);
            return HJ_OK; });

    foreachAllCom([this](std::shared_ptr<HJRteCom> i_com)
                  {
            HJFLogi("{} {} refcnt stat: {}", getDebugName(), i_com->getInsName(), i_com->refCntGet());
            return HJ_OK; });
}
int HJRteGraphProc::runRender()
{
    int i_err = HJ_OK;
    do
    {
        // int64_t t0 = HJCurrentSteadyMS();
        //  if (m_faceMgr)
        //  {
        //      m_faceMgr->procPre();
        //  }

#if defined(HarmonyOS)
        const std::shared_ptr<HJOGRenderEnv> &renderEnv = getRenderEnv();
        if (!renderEnv)
        {
            break;
        }
        renderEnv->makeOffScreenCurrent();
#else
        if (m_makeCurrentCb)
        {
            m_makeCurrentCb(true);
        }
#endif

        HJRteGraph::graphReset();

        if (m_bForceUpdateOnce)
        {
            i_err = priPreCheckGraph();
            if (i_err < 0)
            {
                HJFLoge("{} priPreCheckGraph error", getDebugName());
                break;
            }
            priStatRefCnt();
        }

        HJBaseParam::Ptr param = HJBaseParam::Create();
        bool bRender = false;

        i_err = foreachSource([this, param, &bRender](std::shared_ptr<HJRteCom> i_com)
        {
            int ret = HJ_OK;
            do 
            {
                HJRteComSource::Ptr source = std::dynamic_pointer_cast<HJRteComSource>(i_com);
                if (source)
                {
                    ret = source->update(param);
                    if (ret < 0)
                    {
                        break;
                    }
                    if (m_bMainSourceRenderForce)
                    {            
                        if (source->isMainSource())
                        {
							int w = source->getWidth();
							int h = source->getHeight();
							bool bReady = source->IsStateReady();
							bool bResolutionReady = (w > 0) && (h > 0);
							bRender = bResolutionReady && bReady;
                        }
                    }
                    else
                    {
						bRender = true;
                    }
                }
            } while(false);
            return ret; 
         });
        if (i_err < 0)
        {
            HJFLoge("{} runRender error", getDebugName());
            break;
        }

        // For each source, recursively propagate resolution to downstream chain.
        // If source has dependsOn, align source resolution to the dependency source.
        for (const auto& com : getSources())
        {
            auto source = std::dynamic_pointer_cast<HJRteComSource>(com);
            if (!source)
            {
                continue;
            }

            int w = source->getWidth();
            int h = source->getHeight();

            bool hasDependsOn = false;
            const std::string& dependsOn = source->getDependsOn();
            if (!dependsOn.empty())
            {
                auto depSource = HJRteGraph::findSourceByInsName(dependsOn);
                if (depSource)
                {
                    const int depW = depSource->getWidth();
                    const int depH = depSource->getHeight();
                    if (depW > 0 && depH > 0)
                    {
                        w = depW;
                        h = depH;
                        hasDependsOn = true;
                    }
                }
            }

            if (w <= 0 || h <= 0)
            {
                continue;
            }
            const std::string sourceName = source->getInsName();
            bool resolutionChanged = true;
            auto it = m_lastAdjustedResolutionBySource.find(sourceName);
            if (it != m_lastAdjustedResolutionBySource.end())
            {
                resolutionChanged = (it->second.first != w) || (it->second.second != h);
            }
            //must use m_bForceUpdateOnce, for example, dynamic add filter, the filter fbo w=h=0, must change
            if (resolutionChanged || m_bForceUpdateOnce)
            {
                HJRteGraph::adjustResolutionFromSourceChain(source, w, h);
                m_lastAdjustedResolutionBySource[sourceName] = std::make_pair(w, h);
            }
        }

        if (!bRender)
        {
            HJFLogd("bRender is false return, the fbo w and h is not adjust, so not render");
            break;
        }

        i_err = foreachTarget([this](std::shared_ptr<HJRteCom> i_com)
        {
            int ret = HJ_OK;

            HJRteComDraw::Ptr draw = std::dynamic_pointer_cast<HJRteComDraw>(i_com);
            if (draw)
            {
                if (HJRteGraphDrive::isManulDrive())
                {
                    HJRteDriftInfo::Ptr driftInfo = nullptr;
                    ret = renderFromBottomToTop(i_com, driftInfo);
                }
                else
                {
                    int curFps = draw->getFps();
                    int graphFps = HJRteGraphDrive::getFps();
                    if ((curFps <= 0) || (curFps > graphFps))
                    {
                        curFps = graphFps;
                        if (curFps == 0)
                        {
                            curFps = 30;
                        }
                    }
                    int ratio = graphFps / curFps;
                    if ((m_renderStatIdx % ratio) == 0)
                    {
                        HJRteDriftInfo::Ptr driftInfo = nullptr;
                        ret = renderFromBottomToTop(i_com, driftInfo);
                    }
                }
            }
            return ret; 
        });

        // if (m_faceMgr)
        // {
        //     m_faceMgr->procPost();
        // }

        // int64_t t1 = HJCurrentSteadyMS();
        // HJFLogi("avSync run render timediff:{}", (t1 - t0));

    } while (false);
    return i_err;
}

int HJRteGraphProc::insertFilterAfter(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_dstShader, std::shared_ptr<HJRteComLinkInfo> i_dstLinkInfo)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteGraph::insertFilterAfter(i_com, i_dstCom, i_dstShader, i_dstLinkInfo);
        if (i_err < 0)
        {
            break;
        }
        // setForceAdjustResolution(true);
    } while (false);
    return i_err;
}
std::shared_ptr<HJRteComSourceBridge> HJRteGraphProc::getVideoSourceBridge()
{
    return getUseTypeCom<HJRteComSourceBridge>();
}
std::shared_ptr<HJRteComSource> HJRteGraphProc::priGetMainFilterSource()
{
    HJRteComSource::Ptr oSource = nullptr;
    foreachSource([this, &oSource](std::shared_ptr<HJRteCom> i_com)
        {
            HJRteComSource::Ptr source = std::dynamic_pointer_cast<HJRteComSource>(i_com);
            if (source && source->isMainSource())
            {
                oSource = source;
            }
            return HJ_OK; 
        });
    return oSource;
}
void HJRteGraphProc::clearSourceResolutionCache(const std::string& i_sourceInsName)
{
    if (i_sourceInsName.empty())
    {
        return;
    }
    m_lastAdjustedResolutionBySource.erase(i_sourceInsName);
}
void HJRteGraphProc::setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo)
{
    HJ_LOCK(m_faceMutex);
    m_faceInfoBySource[i_sourceInsName] = i_faceInfo;
}
std::string HJRteGraphProc::getFaceInfo(const std::string& i_sourceInsName)
{
    HJ_LOCK(m_faceMutex);
    auto it = m_faceInfoBySource.find(i_sourceInsName);
    if (it != m_faceInfoBySource.end())
        return it->second;
    return "";
}
void HJRteGraphProc::clearFaceInfo(const std::string& i_sourceInsName)
{
    HJ_LOCK(m_faceMutex);
    m_faceInfoBySource.erase(i_sourceInsName);
}
int HJRteGraphProc::procEGLSurface(const std::shared_ptr<HJOGEGLSurface> &i_eglSurface, int i_type, int i_state)
{
    int i_err = HJ_OK;
    do
    {
        if ((i_state != HJTargetState_Create) && (i_state != HJTargetState_Destroy))
        {
            break;
        }

        i_err = HJRteGraphBaseEGL::asyncOverride([this, i_eglSurface, i_type, i_state]()
                                                 {
            bool bEnable = i_state == HJTargetState_Create ? true : false;
            int fps = i_eglSurface ? i_eglSurface->getFps() : 30;
            if (i_type == HJOGEGLSurfaceType_UI)
            {
                HJRteComDrawEGLUI_0::Ptr uicom = getUseTypeCom<HJRteComDrawEGLUI_0>();
                if (uicom)
                {
                    uicom->setSurface(i_eglSurface);
                    uicom->setEnable(bEnable);
                    uicom->setFps(fps);
                }
            }
            else if (i_type == HJOGEGLSurfaceType_EncoderPusher)
            {
                HJRteComDrawEGLEncoder::Ptr encoder = getUseTypeCom<HJRteComDrawEGLEncoder>();
                if (encoder)
                {
                    encoder->setSurface(i_eglSurface);
                    encoder->setEnable(bEnable);
                    encoder->setFps(fps);
                }
            }
            else if (i_type == HJOGEGLSurfaceType_ImageReceiver)
            {
                HJRteComDrawEGLImgReceiver::Ptr imageReceiver = getUseTypeCom<HJRteComDrawEGLImgReceiver>();
                if (imageReceiver)
                {
                    imageReceiver->setSurface(i_eglSurface);
                    imageReceiver->setEnable(bEnable);
                    imageReceiver->setFps(fps);
                }
            }
            return HJ_OK; });
    } while (false);
    return i_err;
}
int HJRteGraphProc::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        HJFLogi("{} init version:{}", getDebugName(), HJCoreVersion::getVersionDetail());
        HJBaseParam::Ptr renderParam = HJBaseParam::Create();
        if (i_param && i_param->contains("renderListener"))
        {
            m_renderListener = i_param->getValue<HJListener>("renderListener");
        }

        HJ_CatchMapPlainGetVal(i_param, bool, "bMainSourceRenderForce", m_bMainSourceRenderForce);
		HJFLogi("{} init m_bMainSourceRenderForce:{}", getDebugName(), m_bMainSourceRenderForce);

        i_err = HJRteGraphBaseEGL::init(i_param);
        if (i_err < 0)
        {
            if (m_renderListener)
            {
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT)));
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_INIT_FAILED)));
            }
            break;
        }
        i_err = HJRteGraphBaseEGL::syncOverride([this, i_param]()
                                                {
                int ret = 0;
                do
                {                 
                    ret = constructGraph(i_param);
                    if (ret < 0)
                    {
                        HJFLoge("{} constructGraph error", getDebugName());
                        break;
                    }

                    HJRteGraphBaseEGL::setCanRun(true);
                } while (false);
                return ret; });
        if (i_err < 0)
        {
            if (m_renderListener)
            {
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_INIT)));
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_INIT_FAILED)));
            }
            HJFLoge("{} HJRteGraphProc init error", getDebugName());
            break;
        }
        if (m_renderListener)
        {
            m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_INIT_SUCCESS)));
        }
    } while (false);
    return i_err;
}

void HJRteGraphProc::graphProcAsync(std::function<int()> i_func)
{
    HJRteGraphBaseEGL::asyncOverride([this, i_func]()
                                     {
           int i_err = i_func();
           if (i_err < 0)
           {
              HJFLoge("{} HJRteGraphProc graphProc error", getDebugName());
              if (m_renderListener)
              {
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT)));
              }
           }
           return i_err; });
}
void HJRteGraphProc::setGiftPusher(bool i_bGiftPusher)
{
    HJRteGraphBaseEGL::asyncOverride([this, bGiftPusher = i_bGiftPusher]()
                                     {
        HJRteComSourceImgSeq::Ptr pngSeq = getUseTypeCom<HJRteComSourceImgSeq>();
        if (!pngSeq)
        {
            return HJ_OK;
        }
        HJRteComDrawEGLEncoder::Ptr encoder = getUseTypeCom<HJRteComDrawEGLEncoder>();
        if (!encoder)
        {
            return HJ_OK;
        }

        for (auto link : m_links)
        {
            if ((link->getSrcComPtr() == pngSeq) && (link->getDstComPtr() == encoder))
            {
                link->setDrawEnable(bGiftPusher);
                break;
            }
        }
        return HJ_OK; });
}
void HJRteGraphProc::setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape)
{
    HJRteGraphBaseEGL::asyncOverride([this, bDoubleScreen = i_bDoubleScreen, bLandScape = i_bLandscape]()
                                     {
		//fixme after modify, the end, may be not blurFBO;
		HJRteComDrawBlurCascadeFBO::Ptr blurFBO = getUseTypeCom<HJRteComDrawBlurCascadeFBO>();
		if (!blurFBO)
		{
			HJFLogi("{} setDoubleScreen not find blurFBO", getDebugName());
			return HJ_OK;
		}
		foreachTarget([this, blurFBO, bDoubleScreen, bLandScape](std::shared_ptr<HJRteCom> i_com)
			{
				HJRteComDrawEGLUI_0::Ptr drawUI = std::dynamic_pointer_cast<HJRteComDrawEGLUI_0>(i_com);
				if (drawUI)
				{
					breakLinks(blurFBO, drawUI);
					HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
					if (bDoubleScreen)
					{
						HJRectf one;
						HJRectf two;
						if (bLandScape)
						{
							one = HJRectf{ 0, 0, 0.5, 1 };
							two = HJRectf{ 0.5, 0, 0.5, 1 };
						}
						else
						{
							one = HJRectf{ 0, 0, 1, 0.5 };
							two = HJRectf{ 0, 0.5, 1, 0.5 };
						}
						connectCom(blurFBO, drawUI, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(one.x, one.y, one.w, one.h));
						connectCom(blurFBO, drawUI, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(two.x, two.y, two.w, two.h));
					}
					else
					{
						connectCom(blurFBO, drawUI, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0, 0, 1, 1));
					}

					HJRteComSourceImgSeq::Ptr pngSeq = getUseTypeCom<HJRteComSourceImgSeq>();
					if (pngSeq)
					{
						HJRectf rect = pngSeq->getRect();
						HJOGBaseShader::Ptr shaderPreMul2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
						breakLinks(pngSeq, drawUI);
						if (bDoubleScreen)
						{
							HJRectf two;
							if (bLandScape)
							{
								two = HJRectf{ 0.5, 0, 0.5, 1 };
							}
							else
							{
								two = HJRectf{ 0, 0.5, 1, 0.5 };
							}

							float x = two.x + two.w * rect.x;
							float y = two.y + two.h * rect.y;
							float w = two.w * rect.w;
							float h = two.h * rect.h;

							connectCom(pngSeq, drawUI, shaderPreMul2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(x, y, w, h, HJWindowRenderMode_CLIP, false, false));
						}
						else
						{
							connectCom(pngSeq, drawUI, shaderPreMul2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(rect.x, rect.y, rect.w, rect.h, HJWindowRenderMode_CLIP, false, false));
						}
					}

					HJRteComSourceFaceu::Ptr faceu = getUseTypeCom<HJRteComSourceFaceu>();
					if (faceu)
					{
						HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
						breakLinks(faceu, drawUI);
						if (bDoubleScreen)
						{
							HJRectf two;
							if (bLandScape)
							{
								two = HJRectf{ 0.5, 0, 0.5, 1 };
							}
							else
							{
								two = HJRectf{ 0, 0.5, 1, 0.5 };
							}
							connectCom(faceu, drawUI, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(two.x, two.y, two.w, two.h));
						}
						else
						{
							connectCom(faceu, drawUI, shader2D);
						}
					}
				}
				return HJ_OK;
			});
        
        return HJ_OK; });
}
int HJRteGraphProc::openPNGSeq(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        HJRteGraphBaseEGL::asyncOverride([this, i_param]()
                                         {
           int ret = HJ_OK;
           HJRteComSourceImgSeq::Ptr pngSeq = HJRteComSourceImgSeq::Create();
           pngSeq->setRteComType(HJRteComType_Source);
           pngSeq->setMainSource(false);
           HJRteComSourceImgSeq::Wtr filterPNGSeqWtr = pngSeq;
           pngSeq->setNotify([this, filterPNGSeqWtr](HJBaseNotifyInfo::Ptr i_info)
            {
                this->asyncOverride([this, filterPNGSeqWtr, i_info]()
                {
                    if (i_info->getType() == HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)
                    {
                        HJFLogi("{} PNG Complete", getDebugName());
                        HJRteComSourceImgSeq::Ptr pngSeq  = filterPNGSeqWtr.lock();
                        if (pngSeq)
                        {
                            removeRecursiveCom(pngSeq);
                        }
                        
                        if (m_renderListener)
                        {
                            m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)));
                        }    
                    }
                    return 0;
                });
            });
            ret = pngSeq->init(i_param);
            if (ret == HJ_OK)
            {
                foreachTarget([this, pngSeq](std::shared_ptr<HJRteCom> i_com)
                {
                    HJRectf rect = pngSeq->getRect();
                    HJRteComDrawEGLUI_0::Ptr drawUI = std::dynamic_pointer_cast<HJRteComDrawEGLUI_0>(i_com);
                    HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
                    if (drawUI)
                    {
		                connectCom(pngSeq, drawUI, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(rect.x, rect.y, rect.w, rect.h, HJWindowRenderMode_CLIP, false, false));
                    }
                    HJRteComDrawEGLEncoder::Ptr encoder = std::dynamic_pointer_cast<HJRteComDrawEGLEncoder>(i_com);
                    if (encoder)
                    {
		                connectCom(pngSeq, encoder, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(rect.x, rect.y, rect.w, rect.h, HJWindowRenderMode_CLIP, false, false));
                    }
                    HJRteComDrawPBOFBOTarget_0::Ptr endPBO = std::dynamic_pointer_cast<HJRteComDrawPBOFBOTarget_0>(i_com);
                    if (endPBO)
                    {
                        connectCom(pngSeq, endPBO, shader2D, HJRteComLinkInfo::Create<HJRteComLinkInfo>(rect.x, rect.y, rect.w, rect.h, HJWindowRenderMode_CLIP, false, false));
                        /////test
						//HJRteComDrawEGLUI_0::Ptr UI = getUseTypeCom<HJRteComDrawEGLUI_0>();
                        //if (UI)
                        //{
                        //    connectCom(endPBO, UI, shader2D);
                        //}                     
                    }
                    return HJ_OK;
                });
                m_sources.push_back(pngSeq);
            }
            return ret; });
    } while (false);
    return i_err;
}
int HJRteGraphProc::connectCom(HJRteCom::Ptr i_srcCom, HJRteCom::Ptr i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader, std::shared_ptr<HJRteComLinkInfo> i_linkInfo)
{
    return priConnect(i_srcCom, i_dstCom, i_shader, i_linkInfo);
}
int HJRteGraphProc::priConnect(HJRteCom::Ptr i_srcCom, HJRteCom::Ptr i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader, std::shared_ptr<HJRteComLinkInfo> i_linkInfo)
{
    m_links.push_back(i_srcCom->addTarget(i_dstCom, i_shader, i_linkInfo));
    return HJ_OK;
}
void HJRteGraphProc::addSource(std::shared_ptr<HJRteCom> i_com)
{
    m_sources.push_back(std::move(i_com));
    // setForceAdjustResolution(true);
}
void HJRteGraphProc::addTarget(std::shared_ptr<HJRteCom> i_com)
{
    m_targets.push_back(std::move(i_com));
    // setForceAdjustResolution(true);
}
void HJRteGraphProc::addFilter(std::shared_ptr<HJRteCom> i_com)
{
    m_filters.push_back(std::move(i_com));
    // setForceAdjustResolution(true);
}
#if defined(HarmonyOS)
HJOGRenderWindowBridge::Ptr HJRteGraphProc::renderWindowBridgeAcquire()
{
    HJOGRenderWindowBridge::Ptr bridge = nullptr;
    int i_err = HJRteGraphBaseEGL::sync([this, &bridge]()
                                        {
            HJRteComSourceBridge::Ptr videoSource = getVideoSourceBridge();
            if (!videoSource)
            {
                return -1;
            }
            bridge = videoSource->renderWindowBridgeAcquire();
            return 0; });
    if (i_err < 0)
    {
        bridge = nullptr;
    }
    return bridge;
}
HJOGRenderWindowBridge::Ptr HJRteGraphProc::renderWindowBridgeAcquireSoft()
{
    HJOGRenderWindowBridge::Ptr bridge = nullptr;
    int i_err = HJRteGraphBaseEGL::sync([this, &bridge]()
                                        {
           HJRteComSourceBridge::Ptr videoSource = getVideoSourceBridge();
           if (!videoSource)
           {
               return -1;
           }
           bridge = videoSource->renderWindowBridgeAcquireSoft();
           return 0; });
    if (i_err < 0)
    {
        bridge = nullptr;
    }
    return bridge;
}
#endif
void HJRteGraphProc::priDoneOnGraphThread()
{
    m_lastAdjustedResolutionBySource.clear();
}
void HJRteGraphProc::done()
{
    HJFLogi("{} done enter", getDebugName());
    HJRteGraphBaseEGL::done();
    HJFLogi("{} done end-------------", getDebugName());
}
int HJRteGraphProc::priOpenEffect(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        int effectType = HJRteEffect_UNKNOWN;
        HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJRteEffectType), effectType);

        if (effectType == HJRteEffect_Blur)
        {
            foreachUseType<HJRteComDrawBlurCascadeFBO>([](HJRteCom::Ptr i_com)
                                                       { 
		        i_com->setEnable(true);
		        return HJ_OK; });
        }

    } while (false);
    return i_err;
}
int HJRteGraphProc::priCloseEffect(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        int effectType = HJRteEffect_UNKNOWN;
        HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJRteEffectType), effectType);

        if (effectType == HJRteEffect_Blur)
        {
            foreachUseType<HJRteComDrawBlurCascadeFBO>([](HJRteCom::Ptr i_com)
                                                       { 
		        i_com->setEnable(false);
		        return HJ_OK; });
        }
    } while (false);
    return i_err;
}
 int HJRteGraphProc::openFaceEffect()
{
     HJRteGraphBaseEGL::asyncOverride([this]()
     {
        m_bFaceEffectEnable = true;
        return HJ_OK;
     });
     return HJ_OK;
 }
 void HJRteGraphProc::closeFaceEffect()
{
     HJRteGraphBaseEGL::asyncOverride([this]()
     {
		m_bFaceEffectEnable = false;

		HJBaseParam::Ptr param = HJBaseParam::Create();
		HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), HJRteEffect_Blur);
		priCloseEffect(param);

        m_faceStatus = HJFaceStatus_Unknown;
		return HJ_OK;
     });
 }
void HJRteGraphProc::openPBO(HJMediaDataReaderCb i_cb)
{
    HJRteGraphBaseEGL::asyncOverride([this, i_cb]()
                                     {
        foreachUseType<HJRteComDrawPBOFBODetect>([i_cb](HJRteCom::Ptr i_com) 
	    { 
            HJRteComDrawPBOFBODetect::Ptr drawPOBFBO = std::dynamic_pointer_cast<HJRteComDrawPBOFBODetect>(i_com);
            if (drawPOBFBO)
            {
                drawPOBFBO->setReadCb(i_cb);
            }
		    i_com->setEnable(true);
            
		    return HJ_OK;
	    });
        return 0; });
}
void HJRteGraphProc::closePBO()
{
    HJRteGraphBaseEGL::asyncOverride([this]()
                                     {
        foreachUseType<HJRteComDrawPBOFBODetect>([](HJRteCom::Ptr i_com) 
	    { 
            i_com->setEnable(false);
            HJRteComDrawPBOFBODetect::Ptr drawPOBFBO = std::dynamic_pointer_cast<HJRteComDrawPBOFBODetect>(i_com);
            if (drawPOBFBO)
            {
                drawPOBFBO->setReadCb(nullptr);
            }
		    return HJ_OK;
	    });
        return 0; });
}
std::shared_ptr<HJFaceuInfo> HJRteGraphProc::getFaceuInfo()
{
    std::shared_ptr<HJFaceuInfo> faceuInfo = nullptr;
    HJRteGraphBaseEGL::sync([this, &faceuInfo]()
                            {
        foreachUseType<HJRteComSourceFaceu>([&faceuInfo](HJRteCom::Ptr i_com) 
	    { 
            HJRteComSourceFaceu::Ptr faceu = std::dynamic_pointer_cast<HJRteComSourceFaceu>(i_com);
            if (faceu)
            {
                faceuInfo = faceu->getFaceuInfo();
            }
		    return HJ_OK;
	    });
        return 0; });
    return faceuInfo;
}

void HJRteGraphProc::setFaceProtected(bool i_bProtected)
{
    HJRteGraphBaseEGL::asyncOverride([this, i_bProtected]()
                                     {
        m_bFaceProtected = i_bProtected;
        HJFLogi("{} setFaceProtected asyncOverride thread i_bProtect:{}", getDebugName(), i_bProtected);
        if (!m_bFaceProtected)
        {
            m_faceStatus = HJFaceStatus_Unknown;
            HJBaseParam::Ptr param = HJBaseParam::Create();
            HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), HJRteEffect_Blur);
            HJFLogi("{} setFaceProtected asyncOverride thread i_bProtect:{} closeEffect enter", getDebugName(), i_bProtected);
            priCloseEffect(param);          
            HJFLogi("{} setFaceProtected asyncOverride thread i_bProtect:{} closeEffect end", getDebugName(), i_bProtected);
        }
        return HJ_OK; });
}
void HJRteGraphProc::openEffect(HJBaseParam::Ptr i_param)
{
    HJRteGraphBaseEGL::asyncOverride([this, i_param]()
                                     { return priOpenEffect(i_param); });
}
void HJRteGraphProc::closeEffect(HJBaseParam::Ptr i_param)
{
    HJRteGraphBaseEGL::asyncOverride([this, i_param]()
                                     { return priCloseEffect(i_param); });
}
void HJRteGraphProc::setFaceInfo(const std::string& i_sourceInsName, int i_width, int i_height, const std::string &i_faceInfo, bool i_bDebugPoint)
{
    // HJFacePointsEx::Ptr facePt = HJFacePointsEx::Create<HJFacePointsEx>(i_faceWrapper->m_width, i_faceWrapper->m_height, i_faceWrapper->m_faceInfo);
    bool bContainFace = !i_faceInfo.empty();
    HJMoreFacePointsReal::Ptr morePtr = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(i_width, i_height, bContainFace);
    if (bContainFace)
    {
        HJFacePointsInfo faceInfo;
        if (faceInfo.deserial(i_faceInfo) == HJ_OK)
        {
            for (auto &pt : faceInfo.pointItems)
            {
                HJSingleFacePointsReal::Ptr singlePt = HJSingleFacePointsReal::Create();
                singlePt->setFaceRect(HJRectf{(float)pt->rect.left, (float)pt->rect.top, (float)pt->rect.width, (float)pt->rect.height});
                for (auto &item : pt->points)
                {
                    singlePt->add(HJPointf{(float)item->x, (float)item->y});
                }
                morePtr->add(singlePt);
            }
        }
        morePtr->setIsDebugPoints(i_bDebugPoint);
    }

    HJRteGraphBaseEGL::async([this, bContainFace]()
    {
        if (m_bFaceProtected && m_bFaceEffectEnable)
        {
            HJFaceStatus curStatus = bContainFace ? HJFaceStatus_Find : HJFaceStatus_NO_Find;
            if (m_faceStatus != curStatus)
            {
                m_faceStatus = curStatus;
                HJBaseParam::Ptr param = HJBaseParam::Create();
                HJ_CatchMapPlainSetVal(param, int, HJ_CatchName(HJRteEffectType), HJRteEffect_Blur);
                if (m_faceStatus == HJFaceStatus_Find)
                {
                    priCloseEffect(param);
                }
                else
                {
                    priOpenEffect(param);
                }
                forceRefreshOnce();
            }
        }
		return 0;
     });

    morePtr->setSystemTime(HJCurrentSteadyMS());

    HJMoreFacePointsReal::Ptr smoothPoint = m_facePointSmooth->smooth(morePtr);

    if (smoothPoint)
    {
        if (!i_sourceInsName.empty())
        {
            setFaceInfo(i_sourceInsName, smoothPoint->serial());
        }
    }

    // //HJRteGraphBaseEGL::asyncOverride([this, facePt]()
    // HJRteGraphBaseEGL::async([this, facePt]() //not use override, not force update
    // {
    //     if (m_faceMgr)
    //     {
    //         m_faceMgr->setFaceInfo(facePt);
    //     }
    //     return HJ_OK;
    // });
}

NS_HJ_END
