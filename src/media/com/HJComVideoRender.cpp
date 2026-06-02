#include "HJComVideoRender.h"
#include "HJFLog.h"
#include "HJMediaInfo.h"
#include "HJBaseCodec.h"
#include "HJFFHeaders.h"
#include "HJComMediaFrameCvt.h"
#include "HJComEvent.h"
#include "HJTimerSync.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

int HJComVideoRender::s_maxMediaSize = 2;

HJComVideoRender::HJComVideoRender()
{
    HJ_SetInsName(HJComVideoRender);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);

    m_timeSync = HJSysTimerSync::Create();
}
HJComVideoRender::~HJComVideoRender()
{
    HJFLogi("{} ~ enter", m_insName);
}

int HJComVideoRender::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJBaseComAsyncThread::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        if (i_param->contains("HJDeviceType"))
        {
            m_HJDeviceType = i_param->getValInt("HJDeviceType");
        }
        if (m_HJDeviceType == HJDEVICE_TYPE_NONE)
        {
#if defined(HarmonyOS)
            HJ_CatchMapGetVal(i_param, HJOGRenderWindowBridge::Ptr, m_bridge);
#endif
        }
#if defined(HarmonyOS)        
        HJ_CatchMapPlainGetVal(i_param, HJOGRenderWindowBridge::Ptr, "HJOGRenderWindowBridgeSoft", m_softBridge);
#endif
    } while (false);
    return i_err;
}

int HJComVideoRender::priTryDraw(int64_t &o_sleepTime)
{
    int i_err = HJ_OK;
    do
    {
        if (!m_curFrame)
        {
            break;
        }
        i_err = m_timeSync->sync(m_curFrame->getPTS(), o_sleepTime);
        if (i_err == HJ_WOULD_BLOCK)
        {
            break;
        }

        if (m_HJDeviceType == HJDEVICE_TYPE_OHCODEC)
        {
            m_curFrame = nullptr;
//            if (m_curFrame->haveStorage("HJMediaFrameListener"))
//            {
//                HJMediaFrameListener func = m_curFrame->getValue<HJMediaFrameListener>("HJMediaFrameListener");
//                (*m_curFrame)["isOHCodecRender"] = (bool)true;
//                func(m_curFrame);
//            }
        }
        else if (m_HJDeviceType == HJDEVICE_TYPE_NONE)
        {
            if (m_bridge)
            {
                HJAVFrame::Ptr avFrame = m_curFrame->getMFrame();
                AVFrame *frame = avFrame->getAVFrame();
                uint8_t *pData[3] = {};
                pData[0] = frame->data[0];
                pData[1] = frame->data[1];
                pData[2] = frame->data[2];
                int nLineSize[3] = {};
                nLineSize[0] = frame->linesize[0];
                nLineSize[1] = frame->linesize[1];
                nLineSize[2] = frame->linesize[2];
#if defined(HarmonyOS)
                if (m_softBridge && m_bEnableSoft)
                {
                    i_err = m_softBridge->produceFromPixel(HJTransferRenderModeInfo::Create(), pData, nLineSize, frame->width, frame->height);
                    m_bEnableSoft = false;
                } 
                
                if (m_bSoftBridgeTest)
                {
                    HJ_SLEEP(100);
                    m_bSoftBridgeTest = false;
                }
                
                i_err = m_bridge->produceFromPixel(HJTransferRenderModeInfo::Create(), pData, nLineSize, frame->width, frame->height);
#endif
            }
            else
            {
                if (m_notify)
                {
                    HJBaseNotifyInfo::Ptr info = HJBaseNotifyInfo::Create(HJCOMPLAYER_RENDER_FRAME);
                    (*info)[HJ_CatchName(HJMediaFrame::Ptr)] = (HJMediaFrame::Ptr)m_curFrame;
                    m_notify(info);

                    //HJAVFrame::Ptr avFrame = m_curFrame->getMFrame();
                    //AVFrame *frame = avFrame->getAVFrame();
                    // HJFLogi("video render frame: w:{}, h:{}, pts:{}, mediaSize:{} maxMediaSize:{}", frame->width, frame->height, frame->pts, getMediaQueueSize(), getMediaQueueMaxSize());
                }
            }
        }

        m_curFrame = nullptr;
    } while (false);
    return i_err;
}

int HJComVideoRender::run()
{
    int i_err = 0;
    do
    {
        int64_t sleepTime = 0;
        i_err = priTryDraw(sleepTime);
        if (i_err == HJ_WOULD_BLOCK)
        {
            setTimeout(sleepTime);
            break;
        }

        if (!m_mediaFrameDeque.isEmpty())
        {
            m_curFrame = HJComMediaFrameCvt::getFrame(m_mediaFrameDeque.pop());
            if (!m_curFrame)
            {
                i_err = -1;
                break;
            }
            setTimeout(0);
        }
    } while (false);
    return i_err;
}
void HJComVideoRender::done()
{

    HJBaseComAsyncThread::done();
}
void HJComVideoRender::join()
{
    m_bQuit = true;
    HJBaseComAsyncThread::join();
}
NS_HJ_END