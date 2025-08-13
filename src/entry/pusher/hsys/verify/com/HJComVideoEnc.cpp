#include "HJComVideoEnc.h"
#include "HJFLog.h"
#include "HJLog.h"
#include "hsys/HJVEncOHCodec.h"

NS_HJ_BEGIN

HJComVideoEnc::HJComVideoEnc()
{
    HJ_SetInsName(HJComVideoEnc);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);
}
HJComVideoEnc::~HJComVideoEnc()
{
    HJFLogi("{} ~HJComVideoEnc enter", m_insName);
}

int HJComVideoEnc::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }    
        
        if (i_param->contains(HJ_CatchName(HOVideoSurfaceCb)))
        {
            m_surfaceCb = i_param->getValue<HOVideoSurfaceCb>(HJ_CatchName(HOVideoSurfaceCb));
        }
        if (!m_surfaceCb)
        {
            i_err = -1;
            break;
        }    
        
        if (i_param->contains(HJ_CatchName(HJVideoInfo::Ptr)))
        {
            m_videoInfo = i_param->getValue<HJVideoInfo::Ptr>(HJ_CatchName(HJVideoInfo::Ptr));
        }
        if (!m_videoInfo)
        {
            i_err = -1;
            break;
        }    
        
        async([this]()
        {
            int i_err = 0;
            do
            {
                m_encoder = std::dynamic_pointer_cast<HJVEncOHCodec>(HJBaseCodec::createEncoder(HJMEDIA_TYPE_VIDEO, HJDEVICE_TYPE_OHCODEC));
                if (!m_encoder)
                {
                    i_err = -1;
                    break;
                }    
                i_err = m_encoder->init(m_videoInfo);
                if (i_err < 0)
                {
                    break;
                }    
        
                m_nativeWindow = m_encoder->getNativeWindow();
                if (m_nativeWindow && m_surfaceCb)
                {
                    m_surfaceCb(m_nativeWindow, m_videoInfo->m_width, m_videoInfo->m_height, true);
                }    
                
            } while(false);
            return i_err;
        });
    } while (false);
    return i_err;
}
int HJComVideoEnc::run() 
{
    int i_err = 0;
    do 
    {
        if (m_encoder)
        {
            HJMediaFrame::Ptr hjFrame = nullptr;
            i_err = m_encoder->getFrame(hjFrame);
            if (i_err == HJ_WOULD_BLOCK)
            {
                setTimeout(10);
            }    
            else if (i_err == HJ_OK)
            {
                HJComMediaFrame::Ptr frame = HJComMediaFrame::Create();
                frame->SetFlag(MEDIA_VIDEO);
                (*frame)[HJ_CatchName(HJMediaFrame::Ptr)] = hjFrame;
                i_err = send(frame);
                //HJFLogi("{} send ret:{}", m_insName, i_err);
                if (i_err < 0)
                {
                    HJFLoge("send error ret:{}", i_err);
                }
                
                setTimeout(0);
            }        
            else if (i_err < 0)
            {
                HJFLoge("{} error", m_insName);
            }   
        }    
    } while (false);
    return i_err;
}
void HJComVideoEnc::done() 
{
    if (m_nativeWindow && m_surfaceCb)
    {
        HJFLogi("{} setnativewindow done", m_insName);
        m_surfaceCb(m_nativeWindow, 0, 0, false);
    }
    HJBaseComAsyncThread::done();
}

NS_HJ_END