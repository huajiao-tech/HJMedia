#include "HJGraphComVideoCapture.h"
#include "HJFLog.h"
#include "HJComVideoCapture.h"
#include "HJComVideoFilterPNGSeq.h"
#include "HJComEvent.h"
//#include "SLComVideoAlphaVideo.h"

NS_HJ_BEGIN

HJGraphComVideoCapture::HJGraphComVideoCapture()
{
    HJ_SetInsName(HJGraphComVideoCapture);
}
HJGraphComVideoCapture::~HJGraphComVideoCapture()
{
    HJFLogi("{} ~HJGraphComVideoCapture", m_insName);
}

int HJGraphComVideoCapture::init(HJBaseParam::Ptr i_param)
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
            if (i_param->contains("renderListener"))
            {
                m_renderListener = i_param->getValue<HJListener>("renderListener");
            }
        }
		i_err = HJGraphBaseRender::init(renderParam);
		if (i_err < 0)
		{
            if (m_renderListener)
            {
                m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_ERROR_DEFAULT)));
            }  
			break;
		}

        HJGraphBaseRender::setUpdateCb([this]()
        {
            int i_err = 0;
            do
            {
                if (m_videoCapture)
		        {
			        i_err = m_videoCapture->renderUpdate(SEND_TO_OWN);
                }
            } while(false);
            if (i_err < 0)
            {
                if (m_renderListener)
                {
                    m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_ERROR_UPDATE)));
                }  
            }
            return i_err;
            
        });
    
        HJGraphBaseRender::setDrawCb([this](int i_targetWidth, int i_targetHeight)
        {
            int i_err = 0;
            do
            {
                if (m_videoCapture)
		        {
			        i_err = m_videoCapture->renderDraw(i_targetWidth, i_targetHeight, SEND_TO_OWN);
                }
            } while(false);
            if (i_err < 0)
            {
                if (m_renderListener)
                {
                    m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_ERROR_DRAW)));
                }  
            }
            return i_err;
        });
        
		HJGraphBaseRender::sync([this, i_param]()
			{
				int i_err = 0;
				HJFLogi("{} init egl", m_insName);
				do
				{
					m_videoCapture = HJComVideoCapture::Create(true);
					m_videoCapture->setNotify(nullptr);
					addCom(m_videoCapture);
                    
                    HJBaseParam::Ptr captureParam = HJBaseParam::Create();
                    if (i_param && i_param->contains(HJBaseParam::s_paramRenderBridge))
                    {
                        (*captureParam)[HJBaseParam::s_paramRenderBridge] = i_param->getValue<std::string>(HJBaseParam::s_paramRenderBridge);
                    }
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
                        m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_ERROR_VIDEOCAPTURE_INIT)));
                    }
                } 
				return i_err;
			});
	} while (false);
	return i_err;
}

int HJGraphComVideoCapture::openPNGSeq(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
        HJComVideoFilterPNGSeq::Ptr filterPNGSeq = HJComVideoFilterPNGSeq::Create(true);
		filterPNGSeq->setInsName("filterPNGSeq_" + std::to_string(m_pngSegIdx++));
        
        HJComVideoFilterPNGSeq::Wtr filterPNGSeqWtr = filterPNGSeq;
		filterPNGSeq->setNotify([this, filterPNGSeqWtr](int i_nType, const HJBaseNotifyInfo& i_info)
		{
            this->async([this, filterPNGSeqWtr, i_nType]()
            {
                if (HJVIDEOCAPTURE_EVENT_PNGSEQ_COMPLETE == i_nType)
                {
                    HJComVideoFilterPNGSeq::Ptr filterPNGSeqPtr = HJComVideoFilterPNGSeq::GetPtrFromWtr(filterPNGSeqWtr);
                    if (filterPNGSeqPtr)
                    {
                        HJBaseCom::replace(filterPNGSeqPtr);
                        removeCom(filterPNGSeqPtr);
                    }
                    if (m_renderListener)
                    {
                        m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_PNGSEQ_COMPLETE)));
                    }    
                }
                return 0;
            });
        });
		addCom(filterPNGSeq);

        HJBaseCom::Ptr lastCom = HJBaseCom::queryLast(m_videoCapture);
		ComConnect(lastCom, filterPNGSeq);
        HJFLogi("{} connection curName:{} nextName:{}", m_insName, lastCom->getInsName(), filterPNGSeq->getInsName());

		//first target init, then source init
		i_err = filterPNGSeq->init(i_param);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
    
    if (i_err < 0)
    {
        if (m_renderListener)
        {
            m_renderListener(std::move(HJMakeNotification(HJVIDEOCAPTURE_EVENT_ERROR_PNGSEQ_INIT)));
        } 
    }
	return i_err;
}

HJOGRenderWindowBridge::Ptr HJGraphComVideoCapture::renderWindowBridgeAcquire()
{
	HJOGRenderWindowBridge::Ptr bridge = nullptr;
	int i_err = HJBaseGraphComAsyncTimerThread::sync([this, &bridge]()
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
void HJGraphComVideoCapture::done()
{
	HJFLogi("{} done enter", m_insName);

	int i_err = HJGraphBaseRender::sync([this]()
	{
        HJBaseGraphCom::doneAllCom();
        return 0;
    });
    HJGraphBaseRender::done();
	HJFLogi("{} HJGraphComVideoCapture done end-------------", m_insName);
}


//int HJGraphComVideoCapture::openAlphaVideo(HJBaseParam::Ptr i_param)
//{
//int i_err = 0;
//	do
//	{
//      SLComVideoAlphaVideo::Ptr alphaVideo = SLComVideoAlphaVideo::Create(true);
//		alphaVideo->setInsName("SLComVideoAlphaVideo");
//		alphaVideo->setFilterType(SLCOM_FILTER_TYPE_VIDEO);
//      SLComVideoAlphaVideo::Wtr alphaVideoWtr = alphaVideo;
//		alphaVideo->setNotify([this, alphaVideoWtr](int i_nType, const SLBaseNotifyInfo& i_info)
//			{
//				this->async([this, alphaVideoWtr, i_nType]()
//					{
//						if (SLComVideoAlphaVideoNotify_Complete == i_nType)
//						{
//                            SLComVideoAlphaVideo::Ptr alphaVideo = SLComVideoAlphaVideo::GetPtrFromWtr(alphaVideoWtr);
//                            if (alphaVideo)
//                            {
//                                SLBaseCom::replace(alphaVideo);
//                                removeCom(alphaVideo);
//                            }
//
////							if (alphaVideo)
////							{
////								alphaVideo->closePlayer();
////							}
//						}
////                        if (SLComVideoAlphaGiftNotify_AsynceClosePlayerEnd == i_nType)
////                        {
////                            SLBaseCom::replace(alphaVideo);
////                            removeCom(alphaVideo);
////                        }
//						return 0;
//					});
//
//			});
//		addCom(alphaVideo);
//
//        SLBaseCom::Ptr lastCom = SLBaseCom::queryLast(m_videoCapture);
//		ComConnect(lastCom, alphaVideo);
//        HJFLogi("{} connection curName:{} nextName:{}", m_insName, lastCom->getInsName(), alphaVideo->getInsName());
//
//		//first target init, then source init
//		i_err = alphaVideo->init(i_param);
//		if (i_err < 0)
//		{
//			break;
//		}
//	} while (false);
//	return i_err;    
//}


NS_HJ_END