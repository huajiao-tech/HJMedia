#include "HJPrioGraphProc.h"
#include "HJFLog.h"
#include "HJPrioComSourceSeries.h"
#include "HJPrioComSourceSplitScreen.h"
#include "HJPrioComGiftSeq.h"
#include "HJComEvent.h"
#include "HJPrioGraph.h"

#if defined(HarmonyOS)
    #include "HJOGRenderWindowBridge.h"
    #include "HJOGEGLSurface.h"
#endif

NS_HJ_BEGIN

HJPrioGraphProc::HJPrioGraphProc()
{
    HJ_SetInsName(HJPrioGraphProc);
}
HJPrioGraphProc::~HJPrioGraphProc()
{
    HJFLogi("{} ~HJPrioGraphProc", m_insName);
}

int HJPrioGraphProc::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
        int insIdx = 0;
        HJBaseParam::Ptr renderParam = HJBaseParam::Create();
        if (i_param)
        {
            if (i_param->contains(HJBaseParam::s_paramFps))
            {
                (*renderParam)[HJBaseParam::s_paramFps] = i_param->getValInt(HJBaseParam::s_paramFps);
            }    
            if (i_param->contains("renderListener"))
            {
                m_renderListener = i_param->getValue<HJListener>("renderListener");
            }
            
            HJ_CatchMapPlainGetVal(i_param, int, "InsIdx", insIdx);
            
            this->setInsName(getInsName() + "_" + std::to_string(insIdx));
            HJ_CatchMapPlainSetVal(renderParam, int, "InsIdx", (int)insIdx);
            
        }
        HJFLogi("{} HJPrioGraphBaseEGL init enter", m_insName);
		i_err = HJPrioGraphBaseEGL::init(renderParam);
		if (i_err < 0)
		{
            if (m_renderListener)
            {
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_INIT)));
            }  
			break;
		}
        i_err = HJPrioGraphBaseEGL::sync([this, i_param, insIdx]()
			{
				int i_err = 0;
				HJFLogi("{} init egl", m_insName);
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
                    m_videoCapture->setInsName(m_videoCapture->getInsName() + "_" + std::to_string(insIdx));
					m_videoCapture->setNotify(nullptr);
                    HJPrioGraph::insert(m_videoCapture);
                    
                    HJBaseParam::Ptr captureParam = HJBaseParam::Create();
                    HJ_CatchMapPlainSetVal(captureParam, int, "InsIdx", (int)insIdx);
                    HJ_CatchMapPlainSetVal(captureParam, HJListener, "renderListener", m_renderListener);
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
                } 
				return i_err;
			});
        if (i_err < 0)
        {
            HJFLoge("{} HJPrioGraphProc init error", getInsName());
            break;
        }    
	} while (false);
	return i_err;
}
void HJPrioGraphProc::setGiftPusher(bool i_bGiftPusher)
{
    HJPrioGraphBaseEGL::async([this, bGiftPusher = i_bGiftPusher]()
    {
        HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::GetPtrFromWtr(m_pngseqWtr);
        if (filterPNGSeq)
        {
            filterPNGSeq->renderModeClear(HJOGEGLSurfaceType_EncoderPusher);
            if (bGiftPusher)
            {
                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);
            }
        }
        return 0;
    });
}
void HJPrioGraphProc::openEffect(HJBaseParam::Ptr i_param)
{
    HJFLogi("openeffect enter");
    HJPrioGraphBaseEGL::async([this, i_param]()
    {
        if (m_videoCapture)
        {
            HJPrioComSourceSeries::Ptr derive = HJ_CvtDynamic(HJPrioComSourceSeries, m_videoCapture);
            if (!derive)
            {
                HJFLoge("openEffect but is not series");
                return HJ_WOULD_BLOCK;
            }    
            HJFLogi("derive openeffect async enter");
            int i_err = derive->openEffect(i_param);
            if (i_err < 0)
            {
                //notify
            }
        }
        return HJ_OK;
    });
     HJFLogi("openeffect end");
}
void HJPrioGraphProc::closeEffect(HJBaseParam::Ptr i_param)
{
    HJPrioGraphBaseEGL::async([this, i_param]()
    {
        if (m_videoCapture)
        {
            HJPrioComSourceSeries::Ptr derive = HJ_CvtDynamic(HJPrioComSourceSeries, m_videoCapture);
            if (!derive)
            {
                HJFLoge("closeEffect but is not series");
                return HJ_WOULD_BLOCK;
            }  
            derive->closeEffect(i_param);
        }      
        return HJ_OK;
    });    
}
void HJPrioGraphProc::setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape)
{
    HJPrioGraphBaseEGL::async([this, bDoubleScreen = i_bDoubleScreen, bLandScape = i_bLandscape]()
    {
        if (m_videoCapture)
        {
            m_videoCapture->renderModeClear(HJOGEGLSurfaceType_UI);
            if (bDoubleScreen)
            {
                HJTransferRenderModeInfo::Ptr one = HJTransferRenderModeInfo::Create();
                HJTransferRenderModeInfo::Ptr two = HJTransferRenderModeInfo::Create();
                    
                if (bLandScape)
                {
                    one->viewOffx = 0.0;
                    one->viewOffy = 0.0;
                    one->viewWidth = 0.5;
                    one->viewHeight = 1.0;
                        
                    two->viewOffx = 0.5;
                    two->viewOffy = 0.0;
                    two->viewWidth = 0.5;
                    two->viewHeight = 1.0;
                }
                else 
                {
                    one->viewOffx = 0.0;
                    one->viewOffy = 0.0;
                    one->viewWidth = 1.0;
                    one->viewHeight = 0.5;
                        
                    two->viewOffx = 0.0;
                    two->viewOffy = 0.5;
                    two->viewWidth = 1.0;
                    two->viewHeight = 0.5;
                }
                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI, one);
                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI, two);
            }
            else
            {
                m_videoCapture->renderModeAdd(HJOGEGLSurfaceType_UI);
            }
        }
        
        HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::GetPtrFromWtr(m_pngseqWtr);
        if (filterPNGSeq)
        {
            filterPNGSeq->renderModeClear(HJOGEGLSurfaceType_UI);
            if (bDoubleScreen)
            {
                HJTransferRenderModeInfo::Ptr two = HJTransferRenderModeInfo::Create();
                if (bLandScape)
                {                     
                    two->viewOffx = 0.5;
                    two->viewOffy = 0.0;
                    two->viewWidth = 0.5;
                    two->viewHeight = 1.0;
                }
                else 
                {                     
                    two->viewOffx = 0.0;
                    two->viewOffy = 0.5;
                    two->viewWidth = 1.0;
                    two->viewHeight = 0.5;
                }
                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI, two);
            } 
            else 
            {
                filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI);
            }
        }
        
        return 0;
    });
}
int HJPrioGraphProc::openPNGSeq(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
        HJPrioComGiftSeq::Ptr filterPNGSeq = HJPrioComGiftSeq::Create(true);
		filterPNGSeq->setInsName("filterPNGSeq_" + std::to_string(m_pngSegIdx++));

        HJPrioComGiftSeq::Wtr filterPNGSeqWtr = filterPNGSeq;
		filterPNGSeq->setNotify([this, filterPNGSeqWtr](HJBaseNotifyInfo::Ptr i_info)
		{
            this->async([this, filterPNGSeqWtr, i_info]()
            {
                if (i_info->getType() == HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)
                {
                    HJPrioComGiftSeq::Ptr filterPNGSeqPtr = HJPrioComGiftSeq::GetPtrFromWtr(filterPNGSeqWtr);
                    if (filterPNGSeqPtr)
                    {
                        HJPrioGraph::remove(filterPNGSeqPtr);
                    }
                    if (m_renderListener)
                    {
                        m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE)));
                    }    
                }
                return 0;
            });
        });
        HJPrioGraph::insert(filterPNGSeq);
                        
        filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_UI);
        filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderPusher);
        filterPNGSeq->renderModeAdd(HJOGEGLSurfaceType_EncoderRecord);
		//first target init, then source init
		i_err = filterPNGSeq->init(i_param);
		if (i_err < 0)
		{
			break;
		}
        
        m_pngseqWtr = filterPNGSeq;
	} while (false);
    
    if (i_err < 0)
    {
        if (m_renderListener)
        {
            m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_PNGSEQ_INIT)));
        } 
    }
	return i_err;
}
#if defined(HarmonyOS)
HJOGRenderWindowBridge::Ptr HJPrioGraphProc::renderWindowBridgeAcquire()
{
    HJFLogi("{} renderWindowBridgeAcquire enter", m_insName);
	HJOGRenderWindowBridge::Ptr bridge = nullptr;
	int i_err = HJPrioGraphBaseEGL::sync([this, &bridge]()
		{
			if (!m_videoCapture)
			{
				return -1;
			}
            bridge = m_videoCapture->renderWindowBridgeAcquire();
			return 0;
		});
	if (i_err < 0)
	{
		bridge = nullptr;
	}
	return bridge;
}
HJOGRenderWindowBridge::Ptr HJPrioGraphProc::renderWindowBridgeAcquireSoft()
{
    HJFLogi("{} renderWindowBridgeAcquireSoft enter", m_insName);
	HJOGRenderWindowBridge::Ptr bridge = nullptr;
	int i_err = HJPrioGraphBaseEGL::sync([this, &bridge]()
		{
			if (!m_videoCapture)
			{
				return -1;
			}
            bridge = m_videoCapture->renderWindowBridgeAcquireSoft();
			return 0;
		});
	if (i_err < 0)
	{
		bridge = nullptr;
	}
	return bridge;    
}
#endif
void HJPrioGraphProc::done()
{
	HJFLogi("{} done enter", m_insName);
    HJPrioGraphBaseEGL::done();
	HJFLogi("{} HJPrioGraphProc done end-------------", m_insName);
}

NS_HJ_END