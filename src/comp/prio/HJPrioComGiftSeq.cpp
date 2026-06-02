#include "HJPrioComGiftSeq.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJComEvent.h"

#if defined(HarmonyOS)
#include "HJOGEGLSurface.h"
#include "HJOGBaseShader.h"
#endif

NS_HJ_BEGIN

HJPrioComGiftSeq::HJPrioComGiftSeq()
{
    HJ_SetInsName(HJPrioComGiftSeq);  
    setPriority(HJPrioComType_GiftSeq2D);
}
HJPrioComGiftSeq::~HJPrioComGiftSeq()
{
    HJFLogi("~HJComVideoFilterPNGSeq");
}
int HJPrioComGiftSeq::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        HJFLogi("{} init enter", m_insName);
        i_err = HJPrioCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        m_parse = HJImgSeqParse::Create();
        m_parse->setNotify(m_notify);
        i_err = m_parse->init(i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

void HJPrioComGiftSeq::done() 
{
    HJFLogi("{} done", m_insName);   
    if (m_draw)
    {
#if defined(HarmonyOS)
        m_draw->release();
        m_draw = nullptr;
#endif
    } 
    m_parse = nullptr;
    
    HJPrioCom::done();
}

int HJPrioComGiftSeq::update(HJBaseParam::Ptr i_param) 
{
    int i_err = 0;
    do 
    {
        if (m_parse)
        {
            i_err = m_parse->update();
        }
    } while (false);
    return i_err;    
}



int HJPrioComGiftSeq::render(HJBaseParam::Ptr i_param) 
{
    int i_err = 0;
    do 
    {
        if (!m_parse)
        {
            break;
        }
        //HJFLogi("{} render enter loop:{} fps:{} prefix:{}", m_insName, m_configInfo.loops, m_configInfo.fps, m_configInfo.prefix);
#if defined(HarmonyOS)
        HJOGEGLSurface::Ptr surface = nullptr;
        HJ_CatchMapGetVal(i_param, HJOGEGLSurface::Ptr, surface);
        if (!surface)
        {
            i_err = -1;
            break;
        }    

        if (!m_draw)
		{
			m_draw = HJOGCopyShaderStrip::Create();
			i_err = m_draw->init(OGCopyShaderStripFlag_2D);
            if (i_err < 0)
            {
                HJFLogi("Draw init i_err:{}", i_err);
                break;
            }
		}
                   
        if (renderModeIsContain(surface->getSurfaceType()))
        {
            std::vector<HJTransferRenderModeInfo::Ptr>& renderModes = renderModeGet(surface->getSurfaceType());
            for (auto it = renderModes.begin(); it != renderModes.end(); it++)
            {
                HJTransferRenderModeInfo::Ptr renderModeInfo = (*it);
                HJTransferRenderModeInfo::Ptr configViewport = HJTransferRenderModeInfo::Create();
                const HJImgSeqConfigPosInfo& position = m_parse->getPositionInfo();
                configViewport->viewOffx = position.topx;
                configViewport->viewOffy = position.topy;
                configViewport->viewWidth = position.width;
                configViewport->viewHeight = position.height;

                HJTransferRenderViewPortInfo::Ptr viewpotInfo = HJTransferRenderModeInfo::compute(renderModeInfo, configViewport, surface->getTargetWidth(), surface->getTargetHeight());

                glViewport(viewpotInfo->x, viewpotInfo->y, viewpotInfo->width, viewpotInfo->height);
                i_err = m_draw->draw(m_parse->getTextureId(), renderModeInfo->cropMode, m_parse->getWidth(), m_parse->getHeight(), viewpotInfo->width, viewpotInfo->height, m_matrix, true);
            }
            if (i_err < 0)
            {
                break;
            }
        }   
#endif

    } while (false);
    return i_err;    
}

NS_HJ_END