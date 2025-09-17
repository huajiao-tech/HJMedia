#include "HJComVideoEncoder.h"
#include "HJFLog.h"
#include "HJLog.h"
#include "HJMediaInfo.h"
#include "HJBaseCodec.h"

#if defined(HarmonyOS)
#include "HJVEncOHCodec.h"
#endif

NS_HJ_BEGIN

HJComVideoEncoder::HJComVideoEncoder()
{
    HJ_SetInsName(HJComVideoEncoder);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);
}
HJComVideoEncoder::~HJComVideoEncoder()
{
    HJFLogi("{} ~HJComVideoEncoder enter", m_insName);
}

int HJComVideoEncoder::init(HJBaseParam::Ptr i_param)
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
#if defined(HarmonyOS)
                m_baseEncoder = HJBaseCodec::createEncoder(HJMEDIA_TYPE_VIDEO, HJDEVICE_TYPE_OHCODEC);
#endif
                if (!m_baseEncoder)
                {
                    i_err = -1;
                    break;
                }    
                i_err = m_baseEncoder->init(m_videoInfo);
                if (i_err < 0)
                {
                    break;
                }    
#if defined(HarmonyOS)        
                HJ_CvtBaseToDerived(HJVEncOHCodec, m_baseEncoder);
                m_nativeWindow = the->getNativeWindow();
                if (m_nativeWindow && m_surfaceCb)
                {
                    m_surfaceCb(m_nativeWindow, m_videoInfo->m_width, m_videoInfo->m_height, true);
                }    
#endif
                
            } while(false);
            return i_err;
        });
    } while (false);
    return i_err;
}
int HJComVideoEncoder::run() 
{
    int i_err = 0;
    do 
    {
        if (m_baseEncoder)
        {
            HJMediaFrame::Ptr hjFrame = nullptr;
            i_err = m_baseEncoder->getFrame(hjFrame);
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
void HJComVideoEncoder::done()
{
    if (m_nativeWindow && m_surfaceCb)
    {
        HJFLogi("{} setnativewindow done", m_insName);
        m_surfaceCb(m_nativeWindow, 0, 0, false);
    }
    HJBaseComAsyncThread::done();
}

NS_HJ_END