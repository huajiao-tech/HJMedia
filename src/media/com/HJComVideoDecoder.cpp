#include "HJComVideoDecoder.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJBaseCodec.h"
#include "HJVDecFFMpeg.h"
#include "HJComMediaFrameCvt.h"
#include "HJVidKeyStrategy.h"

#if defined(HarmonyOS)
    #include "HJOGRenderWindowBridge.h"
    #include "HJVDecOHCodec.h"
#endif

NS_HJ_BEGIN

int HJComVideoDecoder::s_maxMediaSize = 60;

HJComVideoDecoder::HJComVideoDecoder()
{
    HJ_SetInsName(HJComVideoDecoder);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);
}
HJComVideoDecoder::~HJComVideoDecoder()
{
    HJFLogi("{} ~ enter", m_insName);
}

int HJComVideoDecoder::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        if (!i_param)
        {
            i_err = -1;
            break;
        }
        
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }    
        
        HJ_CatchMapPlainGetVal(i_param, int, "HJDeviceType", m_HJDeviceType);
        HJ_CatchMapPlainGetVal(i_param, bool, "IsUseKeyStrategy", m_bUseKeyStrategy);
        if (m_bUseKeyStrategy)
        {
            m_keyStrategy = HJVidKeyStrategy::Create();
        }    
        
        if (m_HJDeviceType == HJDEVICE_TYPE_OHCODEC)
        {
#if defined(HarmonyOS)
            HJ_CatchMapGetVal(i_param, HJOGRenderWindowBridge::Ptr, m_bridge);
            if (!m_bridge)
            {
                i_err = HJErrFatal;
                break;
            }    
            //(*streamInfo)[HJ_CatchName(HJOGRenderWindowBridge::Ptr)] = bridge;
#endif
        } 
        async([this, i_param]()
        {
            m_bReady = true;
            return HJ_OK;
        });
    } while (false);
    return i_err;
}

int HJComVideoDecoder::priTryDrainOuter()
{
    int i_err = HJ_OK;
    do
    {
        if (!m_baseDecoder)
        {
            break;
        }    
        HJMediaFrame::Ptr outFrame = nullptr;
        i_err = m_baseDecoder->getFrame(outFrame);
        if ((i_err == HJ_OK) && outFrame)
        {
            if (m_keyStrategy && m_keyStrategy->filter())
            {
                HJFLogi("video filter not send pts:{} dts:{}", outFrame->getPTS(), outFrame->getDTS());
                //drop the duplicate key frame;
                if (m_HJDeviceType == HJDEVICE_TYPE_OHCODEC)
                {
                    if (outFrame->haveStorage(HJ_CatchName(HJVDecOHCodecResObj::Ptr)))
                    {
#if defined(HarmonyOS)
                        HJVDecOHCodecResObj::Ptr obj = outFrame->getValue<HJVDecOHCodecResObj::Ptr>(HJ_CatchName(HJVDecOHCodecResObj::Ptr));
                        obj->setRender(false);
#endif
                    }
                    outFrame = nullptr;
                }
            }    
            else 
            {
                HJComMediaFrame::Ptr senFrame = HJComMediaFrameCvt::getComFrame(MEDIA_VIDEO, outFrame);
                //HJFLogi("decoder raw frame: {}", outFrame->getPTS());
                i_err = send(senFrame);
                if (i_err < 0)
                {
                    break;
                }
            }
            i_err = HJ_AGAIN;
        }
        else
        {
            break;
        }
    } while (false);
    return i_err;
}
int HJComVideoDecoder::priResetCodec(const std::shared_ptr<HJMediaFrame>& i_frame)
{
    int i_err = HJ_OK;
    do 
    {
        if (m_baseDecoder)
        {
            m_baseDecoder->done();
            m_baseDecoder = nullptr;
        }    
        m_baseDecoder = HJBaseCodec::createVDecoder((HJDeviceType)m_HJDeviceType);
        
        HJStreamInfo::Ptr streamInfo = i_frame->getInfo();
        
        if ((m_HJDeviceType == HJDEVICE_TYPE_OHCODEC) && m_bridge)
        {
#if defined(HarmonyOS)
            (*streamInfo)[HJ_CatchName(HJOGRenderWindowBridge::Ptr)] = m_bridge;
#endif
        }
        i_err = m_baseDecoder->init(streamInfo);
        if (i_err < 0)
        {
            break;
        }
        
    } while (false);
    return i_err;
}
int HJComVideoDecoder::priTryInput()
{
    int i_err = HJ_OK;
    do
    {
        //HJFLogi("priTryInput enter");
        if (m_mediaFrameDeque.isEmpty())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        HJVDecOHCodec::Ptr hwDec = HJ_CvtDynamic(HJVDecOHCodec, m_baseDecoder);
        if (hwDec)
        {
            if (!hwDec->isCanInput())
            {
                i_err = HJ_WOULD_BLOCK;
                break;
            }    
        }    
#endif 
        
		HJMediaFrame::Ptr packetFrame = HJComMediaFrameCvt::getFrame(m_mediaFrameDeque.pop());
		if (!packetFrame)
		{
			i_err = -1;
            HJFLoge("HJVidKeyStrategy getFrame error");
			break;
		}
        
        if (packetFrame->isFlushFrame())
        {
            HJFLogi("flush frame enter pts:{} dts:{}", packetFrame->getPTS(), packetFrame->getDTS());
            i_err = priResetCodec(packetFrame);
            if (i_err < 0)
            {
                HJFLogi("reset codec error");
                break;
            }    
        }    
        
//        if (packetFrame->isVideo())
//        {
//            AVPacket* pk = packetFrame->getAVPacket();
//            if (pk)
//            {
//                HJFLogi("testIsMatch comdecoder pts:{}, dts:{} size:{} isKey:{} mediaSize:{} maxMediaSize:{}", packetFrame->getPTS(),packetFrame->getDTS(), pk->size, packetFrame->isKeyFrame(), getMediaQueueSize(), getMediaQueueMaxSize());
//            }
//        }
        if (HJFRAME_EOF == packetFrame->getFrameType())
        {
            HJFLogi("eof packet enter");
        }  
        
        
        if (m_keyStrategy && m_keyStrategy->isAvaiable())
        {
            HJFLogi("HJVidKeyStrategy process"); 
            m_keyStrategy->process(packetFrame, [this, packetFrame]()
            {
                int ret = HJ_OK;
                do 
                {
                    ret = m_baseDecoder->run(packetFrame);
                    if (ret < 0)
                    {
                        break;
                    }
                    ret = priTryDrainOuter();
                    if (ret == HJ_AGAIN)
                    {
                        ret = HJ_CLOSE;
                        break;
                    }
                    if (ret < 0)
                    {
                        break;
                    }
                } while (false);
                return ret;
            });
        }    
        else 
        {
            //HJFLogi("HJVidKeyStrategy run"); 
            i_err = m_baseDecoder->run(packetFrame);
        }    
		
        
		//HJFLogi("HJVidKeyStrategy video decoder end i_err:{} ", i_err);
		if (i_err < 0)
		{
            HJFLogi("video decoder error i_err:{} ", i_err);
			break;
		}

    } while (false);
    return i_err;
}
int HJComVideoDecoder::run()
{
    int i_err = 0;
    do 
    {       
        if (!m_bReady)
        {
            break;
        }
        
		if (isNextFull(getSharedFrom(this)))
		{
			i_err = HJ_WOULD_BLOCK;
			break;
		}

		i_err = priTryDrainOuter(); //drain every frame
		if (i_err == HJ_AGAIN)
		{
			setTimeout(0);
            i_err = HJ_OK;
			break;
		}
		else if (i_err < 0)
		{
			break;
		}

		i_err = priTryInput();      
		if (i_err == HJ_OK)
		{
			setTimeout(0);
			break;
		}
        
    } while (false);
    return i_err;
}
void HJComVideoDecoder::done()
{
    if (m_keyStrategy)
    {
        m_keyStrategy->setQuit();
    }    
    sync([this]()
    {
        if (m_baseDecoder)
        {
            m_baseDecoder->done();
        }
        return 0;
    });

    HJBaseComAsyncThread::done();
}

NS_HJ_END