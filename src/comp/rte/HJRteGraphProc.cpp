#pragma once

#include "HJFLog.h"
#include "HJRteCom.h"
#include "HJRteGraphProc.h"
#include "HJRteComSourceBridge.h"

#if defined(HarmonyOS)
    #include "HJOGRenderWindowBridge.h"
    #include "HJOGEGLSurface.h"
    #include "HJOGRenderEnv.h"
#endif

NS_HJ_BEGIN

HJRteGraphProc::HJRteGraphProc()
{
    HJ_SetInsName(HJRteGraphProc);
}
HJRteGraphProc::~HJRteGraphProc()
{
    HJFLogi("{} ~HJRteGraphProc", m_insName);
}


int HJRteGraphProc::testRender(std::shared_ptr<HJRteCom> i_com)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        const std::shared_ptr<HJOGRenderEnv>& renderEnv = getRenderEnv();
        const HJOGEGLSurfaceQueue& queue = renderEnv->getEglSurfaceQueue();
        HJOGEGLSurface::Ptr eglSurface = *queue.begin();
        i_err = renderEnv->testMakeCurrent(eglSurface->getEGLSurface());
        if (i_err < 0)
        {
            HJFLoge("makecurrent error");
            break;
        }
        i_err = i_com->foreachPreLink([this, targetWidth = eglSurface->getTargetWidth(), targetHeight = eglSurface->getTargetHeight()](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
                do
                {
                    std::string linkName = i_link->getInsName();
                    const std::shared_ptr<HJRteComLinkInfo>& linkInfo = i_link->getLinkInfo();
                    HJTransferRenderModeInfo::Ptr info = HJTransferRenderModeInfo::Create();
                    if (linkInfo)
                    {
                        info->viewOffx = linkInfo->m_x;
                        info->viewOffy = linkInfo->m_y;
                        info->viewWidth = linkInfo->m_width;
                        info->viewHeight = linkInfo->m_height;
                    }

                
                    HJBaseParam::Ptr param = HJBaseParam::Create();
                    HJ_CatchMapPlainSetVal(param, int, "targetWidth", targetWidth);
                    HJ_CatchMapPlainSetVal(param, int, "targetHeight", targetHeight);
                    HJ_CatchMapSetVal(param, HJTransferRenderModeInfo::Ptr, info);
                    m_videoSource->render(param);
                
                    HJFLogi("{} com:<{} - {}> info:{} render sub #########", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                } while(false);
                return ret;
            });
        if (i_err < 0)
        {
            break;
        }
        
        i_err = renderEnv->testSwap(eglSurface->getEGLSurface());
        if (i_err < 0)
        {
            HJFLoge("makecurrent error");
            break;
        }
#endif
    } while (false);
    return i_err;
}
int HJRteGraphProc::testNotifyAndTryRender(std::shared_ptr<HJRteCom> i_header)
{
    int i_err = HJ_OK;
    do 
    {
        std::string comName = i_header->getInsName();
        HJFLogi("{} com:{} enter", getInsName(), comName);
        //notify all next links ready
        i_err = i_header->foreachNextLink([this](const std::shared_ptr<HJRteComLink>& i_link)
            {
                int ret = HJ_OK;
                do
                {
                    std::string linkName = i_link->getInsName();
                    HJFLogi("{} notify com:<{} - {}> ready", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                    //notify next ready
                    i_link->setReady(true);

                    //com may be have more input, so must just all ready, then render
                    HJRteCom::Ptr com = i_link->getDstComPtr();
                    if (com && com->isAllPreReady())
                    {
                        std::string comName = com->getInsName();
                        HJFLogi("{} com:<{} - {}> info:{} render =======================", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());
                        //pri render to target
                        ret = testRender(com);
                        if (ret < 0)
                        {
                            HJFLoge("{} com:<{} - {}>  render error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                            break;
                        }

                        ret = testNotifyAndTryRender(com);
                        if (ret < 0)
                        {
                            HJFLoge("{} com:<{} - {}> priNotifyAndTryRender error", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                            break;
                        }
                    }
                    else
                    {
                        HJFLogi("{} com:<{} - {}> not ready, so not render---->", getInsName(), i_link->getSrcComName(), i_link->getDstComName());
                    }
                } while (false);           
                return ret;
            });

        HJFLogi("{} com:{} end i_err:{}", getInsName(), comName, i_err);

        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

int HJRteGraphProc::run()
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        const std::shared_ptr<HJOGRenderEnv>& renderEnv = getRenderEnv();
        if (!renderEnv)
        {
            break;
        }    
        const HJOGEGLSurfaceQueue& queue = renderEnv->getEglSurfaceQueue();
        if (queue.empty())
        {
            break;
        }
        
        HJOGEGLSurface::Ptr eglSurface = *queue.begin();
        i_err = renderEnv->testMakeCurrent(eglSurface->getEGLSurface());
        
        graphReset();
        HJBaseParam::Ptr param = HJBaseParam::Create();
        i_err = foreachSource([this, param](std::shared_ptr<HJRteCom> i_com)
        {
            int ret = HJ_OK;
            do 
            {
                ret = m_videoSource->update(param);
                if (ret < 0)
                {
                    break;
                }    
                if (m_videoSource->IsStateReady())
                {
                    HJRteDriftInfo::Ptr driftInfo = HJRteDriftInfo::Create<HJRteDriftInfo>(m_videoSource->getTextureId(), m_videoSource->getWidth(), m_videoSource->getHeight(), HJRteTextureType_OES);
                    ret = testNotifyAndTryRender(i_com);
                    if (ret < 0)
                    {
                        break;
                    }
                }
            } while (false);
            return ret;
        });

#else
        //fixme lfs
        //1. pb make_current offscreen

        //2. reset 
        graphReset();

        //3. loop
		HJBaseParam::Ptr param = HJBaseParam::Create();
		i_err = foreachSource([this, param](std::shared_ptr<HJRteCom> i_com)
			{
				int ret = HJ_OK;
				do
				{
					ret = m_videoSource->update(param);
					if (ret < 0)
					{
						break;
					}
					if (m_videoSource->IsStateReady())
					{
						ret = testNotifyAndTryRender(i_com);
						if (ret < 0)
						{
							break;
						}
					}
				} while (false);
				return ret;
			});

#endif
    } while (false);
    return i_err;
}
int HJRteGraphProc::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        HJBaseParam::Ptr renderParam = HJBaseParam::Create();
        if (i_param)
        {
            if (i_param->contains(HJBaseParam::s_paramFps))
            {
                (*renderParam)[HJBaseParam::s_paramFps] = i_param->getValInt(HJBaseParam::s_paramFps);
            }
            //if (i_param->contains("renderListener"))
            //{
            //    m_renderListener = i_param->getValue<HJListener>("renderListener");
            //}
        }
        i_err = HJRteGraphBaseEGL::init(renderParam);
        if (i_err < 0)
        {
            //if (m_renderListener)
            //{
            //    m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT)));
            //}
            break;
        }
        i_err = HJRteGraphBaseEGL::sync([this, i_param]()
            {
                int ret = 0;
                do
                {
                    m_videoSource = HJRteComSourceBridge::Create();
                    m_sources.push_back(m_videoSource);
                    m_coms.push_back(m_videoSource);
                
                    HJRteCom::Ptr uicom = HJRteCom::Create();
                    uicom->setInsName("uicom");
                    uicom->setPriority(HJRteComPriority_Target);
                    m_coms.push_back(uicom);
                    
                    m_links.push_back(m_videoSource->addTarget(uicom, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.0, 1.0, 0.5)));
                    m_links.push_back(m_videoSource->addTarget(uicom, HJRteComLinkInfo::Create<HJRteComLinkInfo>(0.0, 0.5, 1.0, 0.5)));
                
                } while (false);
                return ret;
            
            
                /*HJFLogi("{} init egl", m_insName);
                do
                {
                    int sourceType = HJPrioComSourceType_UNKNOWN;
                    HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJPrioComSourceType), sourceType);
                    if (sourceType == HJPrioComSourceType_UNKNOWN)
                    {
                        i_err = HJErrInvalidParams;
                        HJFLoge("{} sourceType not find error", getInsName());
                        break;
                    }
                    m_videoCapture = HJPrioComSourceBridge::CreateFactory(HJPrioComSourceType(sourceType));

                    m_videoCapture->setNotify(nullptr);
                    HJPrioGraph::insert(m_videoCapture);

                    HJBaseParam::Ptr captureParam = HJBaseParam::Create();
                    m_videoCapture->renderModeClearAll();
                    m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI);
                    m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);
                    m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);

                    i_err = m_videoCapture->init(captureParam);
                    if (i_err < 0)
                    {
                        break;
                    }
                } while (false);

                if (i_err < 0)
                {
                    if (m_renderListener)
                    {
                        m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_INIT)));
                    }
                }*/
            });
        if (i_err < 0)
        {
            HJFLoge("{} HJRteGraphProc init error", getInsName());
            break;
        }
    } while (false);
    return i_err;
}
void HJRteGraphProc::setGiftPusher(bool i_bGiftPusher)
{
    //HJRteGraphBaseEGL::async([this, bGiftPusher = i_bGiftPusher]()
    //    {
    //        HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::GetPtrFromWtr(m_pngseqWtr);
    //        if (filterPNGSeq)
    //        {
    //            filterPNGSeq->renderModeClear(HJOGEGLSurfaceType_EncoderPusher);
    //            if (bGiftPusher)
    //            {
    //                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);
    //            }
    //        }
    //        return 0;
    //    });
}
void HJRteGraphProc::openEffect(HJBaseParam::Ptr i_param)
{
    //HJFLogi("openeffect enter");
    //HJRteGraphBaseEGL::async([this, i_param]()
    //    {
    //        if (m_videoCapture)
    //        {
    //            HJPrioComSourceSeries::Ptr derive = HJ_CvtDynamic(HJPrioComSourceSeries, m_videoCapture);
    //            if (!derive)
    //            {
    //                HJFLoge("openEffect but is not series");
    //                return HJ_WOULD_BLOCK;
    //            }
    //            HJFLogi("derive openeffect async enter");
    //            int i_err = derive->openEffect(i_param);
    //            if (i_err < 0)
    //            {
    //                //notify
    //            }
    //        }
    //        return HJ_OK;
    //    });
    //HJFLogi("openeffect end");
}
void HJRteGraphProc::closeEffect(HJBaseParam::Ptr i_param)
{
    //HJRteGraphBaseEGL::async([this, i_param]()
    //    {
    //        if (m_videoCapture)
    //        {
    //            HJPrioComSourceSeries::Ptr derive = HJ_CvtDynamic(HJPrioComSourceSeries, m_videoCapture);
    //            if (!derive)
    //            {
    //                HJFLoge("closeEffect but is not series");
    //                return HJ_WOULD_BLOCK;
    //            }
    //            derive->closeEffect(i_param);
    //        }
    //        return HJ_OK;
    //    });
}
void HJRteGraphProc::setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape)
{
    //HJRteGraphBaseEGL::async([this, bDoubleScreen = i_bDoubleScreen, bLandScape = i_bLandscape]()
    //    {
    //        if (m_videoCapture)
    //        {
    //            m_videoCapture->renderModeClear(HJOGEGLSurfaceType_UI);
    //            if (bDoubleScreen)
    //            {
    //                HJTransferRenderModeInfo::Ptr one = HJTransferRenderModeInfo::Create();
    //                HJTransferRenderModeInfo::Ptr two = HJTransferRenderModeInfo::Create();

    //                if (bLandScape)
    //                {
    //                    one->viewOffx = 0.0;
    //                    one->viewOffy = 0.0;
    //                    one->viewWidth = 0.5;
    //                    one->viewHeight = 1.0;

    //                    two->viewOffx = 0.5;
    //                    two->viewOffy = 0.0;
    //                    two->viewWidth = 0.5;
    //                    two->viewHeight = 1.0;
    //                }
    //                else
    //                {
    //                    one->viewOffx = 0.0;
    //                    one->viewOffy = 0.0;
    //                    one->viewWidth = 1.0;
    //                    one->viewHeight = 0.5;

    //                    two->viewOffx = 0.0;
    //                    two->viewOffy = 0.5;
    //                    two->viewWidth = 1.0;
    //                    two->viewHeight = 0.5;
    //                }
    //                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI, one);
    //                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI, two);
    //            }
    //            else
    //            {
    //                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI);
    //            }
    //        }

    //        HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::GetPtrFromWtr(m_pngseqWtr);
    //        if (filterPNGSeq)
    //        {
    //            filterPNGSeq->renderModeClear(HJOGEGLSurfaceType_UI);
    //            if (bDoubleScreen)
    //            {
    //                HJTransferRenderModeInfo::Ptr two = HJTransferRenderModeInfo::Create();
    //                if (bLandScape)
    //                {
    //                    two->viewOffx = 0.5;
    //                    two->viewOffy = 0.0;
    //                    two->viewWidth = 0.5;
    //                    two->viewHeight = 1.0;
    //                }
    //                else
    //                {
    //                    two->viewOffx = 0.0;
    //                    two->viewOffy = 0.5;
    //                    two->viewWidth = 1.0;
    //                    two->viewHeight = 0.5;
    //                }
    //                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI, two);
    //            }
    //            else
    //            {
    //                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI);
    //            }
    //        }

    //        return 0;
    //    });
}
int HJRteGraphProc::openPNGSeq(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    //do
    //{
    //    HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::Create(true);
    //    filterPNGSeq->setInsName("filterPNGSeq_" + std::to_string(m_pngSegIdx++));

    //    HJPrioComGiftSeq::Wtr filterPNGSeqWtr = filterPNGSeq;
    //    filterPNGSeq->setNotify([this, filterPNGSeqWtr](HJBaseNotifyInfo::Ptr i_info)
    //        {
    //            this->async([this, filterPNGSeqWtr, i_info]()
    //                {
    //                    if (i_info->getType() == HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)
    //                    {
    //                        HJPrioComGiftSeq::Ptr filterPNGSeqPtr = HJPrioComGiftSeq::GetPtrFromWtr(filterPNGSeqWtr);
    //                        if (filterPNGSeqPtr)
    //                        {
    //                            HJPrioGraph::remove(filterPNGSeqPtr);
    //                        }
    //                        if (m_renderListener)
    //                        {
    //                            m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)));
    //                        }
    //                    }
    //                    return 0;
    //                });
    //        });
    //    HJPrioGraph::insert(filterPNGSeq);

    //    filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI);
    //    filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);
    //    filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderRecord);
    //    //first target init, then source init
    //    i_err = filterPNGSeq->init(i_param);
    //    if (i_err < 0)
    //    {
    //        break;
    //    }

    //    m_pngseqWtr = filterPNGSeq;
    //} while (false);

    //if (i_err < 0)
    //{
    //    if (m_renderListener)
    //    {
    //        m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_PNGSEQ_INIT)));
    //    }
    //}
    return i_err;
}
#if defined(HarmonyOS)
HJOGRenderWindowBridge::Ptr HJRteGraphProc::renderWindowBridgeAcquire()
{
    HJOGRenderWindowBridge::Ptr bridge = nullptr;
    int i_err = HJRteGraphBaseEGL::sync([this, &bridge]()
        {
            if (!m_videoSource)
            {
                return -1;
            }
            bridge = m_videoSource->renderWindowBridgeAcquire();
            return 0;
        });
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
//            if (!m_videoCapture)
//            {
//                return -1;
//            }
//            bridge = m_videoCapture->renderWindowBridgeAcquireSoft();
            return 0;
        });
    if (i_err < 0)
    {
        bridge = nullptr;
    }
    return bridge;
}
#endif
void HJRteGraphProc::done()
{
    HJFLogi("{} done enter", m_insName);
    HJRteGraphBaseEGL::done();
    HJFLogi("{} HJRteGraphProc done end-------------", m_insName);
}


NS_HJ_END



