#include "HJPrioComSourceSeries.h"
#include "HJFLog.h"
#include "HJPrioGraph.h"
#include "HJPrioComFBOGray.h"
#include "HJPrioComFBOBlur.h"
#include "HJPrioUtils.h"
#include "HJPrioComFBOBase.h"
#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGCopyShaderStrip.h"
#include "HJOGEGLSurface.h"
#endif

NS_HJ_BEGIN

HJPrioComSourceSeries::HJPrioComSourceSeries()
{
	HJ_SetInsName(HJPrioComSourceSeries);
    m_fbo = HJPrioComFBOBase::Create();
}

HJPrioComSourceSeries::~HJPrioComSourceSeries()
{
	HJFLogi("~{}", getInsName());
}

int HJPrioComSourceSeries::openEffect(HJBaseParam::Ptr i_param)
{
#define HJOpenEffectOpenProc(T) \
{\
    HJPrioComFBOBase::Ptr fbo = T::Create();\
	m_graph->insert(fbo); \
	i_err = fbo->init(i_param);\
	if (i_err < 0) \
	{ \
		break;\
	}\
}\

    int i_err = HJ_OK;
    do 
    {
        int effectType = HJPrioEffect_UNKNOWN;
        HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJPrioEffectType), effectType);
        
        if (effectType == HJPrioEffect_Gray)
        {
#if defined(HarmonyOS)
			HJOpenEffectOpenProc(HJPrioComFBOGray);
#endif
        }
        else if (effectType == HJPrioEffect_Blur)
        {
#if defined(HarmonyOS)
			HJOpenEffectOpenProc(HJPrioComFBOBlur);
            HJOpenEffectOpenProc(HJPrioComFBOBlur);
#endif
        }     
    } while (false);
    return i_err;
    
    
}
void HJPrioComSourceSeries::closeEffect(HJBaseParam::Ptr i_param)
{
#define HJOpenEffectCloseProc(T) \
	i_err = m_graph->foreachUseType<T>([this](HJPrioCom::Ptr i_com) \
	{ \
		m_graph->remove(i_com);\
		return HJ_OK;\
	});\
    
    int i_err = HJ_OK;
	do 
	{
		int effectType = HJPrioEffect_UNKNOWN;
		HJ_CatchMapPlainGetVal(i_param, int, HJ_CatchName(HJPrioEffectType), effectType);

		if (effectType == HJPrioEffect_Gray)
		{
#if defined(HarmonyOS)
			HJOpenEffectCloseProc(HJPrioComFBOGray)
#endif
		}
		else if (effectType == HJPrioEffect_Blur)
		{
#if defined(HarmonyOS)
			HJOpenEffectCloseProc(HJPrioComFBOBlur)
#endif
		}
	} while (false);
	
	//fixme
	//if (i_err < 0)
	//{
		//notify 
	//}
}

int HJPrioComSourceSeries::update(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
        if (!i_param)
        {
            i_err = HJErrInvalidParams;
            break;
        }    
		if (m_graph)
		{
            if (!HJPrioComSourceBridge::IsStateAvaiable())
            {
                break;
            }    
            i_err = HJPrioComSourceBridge::update(i_param);
            if (i_err < 0)
            {
                break;
            }    
#if defined(HarmonyOS)
            m_fbo->check(HJPrioComSourceBridge::getWidth(), HJPrioComSourceBridge::getHeight());
            m_fbo->draw([this, i_param]()
            {
                int ret = HJPrioComSourceBridge::getBridge()->draw(HJTransferRenderModeInfo::Create(), HJPrioComSourceBridge::getWidth(), HJPrioComSourceBridge::getHeight());
                if (ret == HJ_OK)
                {
#if defined (HarmonyOS)    
                    i_param->remove(HJ_CatchName(HJPrioComBaseFBOInfo::Ptr));
                    m_LastFboInfo = HJPrioComBaseFBOInfo::Create<HJPrioComBaseFBOInfo>(m_fbo->width(), m_fbo->height(), m_fbo->getMatrix(), m_fbo->texture());
                    HJ_CatchMapSetVal(i_param, HJPrioComBaseFBOInfo::Ptr, m_LastFboInfo);
#endif
                }
                HJPrioComSourceBridge::stat();
                return ret;
            });
#endif
            
			i_err = m_graph->foreach([this, i_param](std::shared_ptr<HJPrioCom> i_com)
				{
                    int ret = HJ_OK;
                    ret = i_com->update(i_param);
                    if (ret < 0)
                    {
                        return ret;
                    }
                    ret = i_com->render(i_param);
                    if (ret < 0)
                    {
                        return ret;
                    }
                    i_param->remove(HJ_CatchName(HJPrioComBaseFBOInfo::Ptr));
                    if (ret == HJ_OK)
                    {                     
                       HJPrioComFBOBase::Ptr fbo = std::dynamic_pointer_cast<HJPrioComFBOBase>(i_com);
                       if (fbo)
                       {
#if defined (HarmonyOS)    
                           m_LastFboInfo = HJPrioComBaseFBOInfo::Create<HJPrioComBaseFBOInfo>(fbo->width(), fbo->height(), fbo->getMatrix(), fbo->texture());
                           HJ_CatchMapSetVal(i_param, HJPrioComBaseFBOInfo::Ptr, m_LastFboInfo);
#endif
                        }                    
                    }
                    return ret;
				});
		}
	} while (false);
	return i_err;
}
int HJPrioComSourceSeries::render(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
#if defined(HarmonyOS)
		if (!m_LastFboInfo)
		{
			i_err = HJ_WOULD_BLOCK;
			break;
		}
		HJOGEGLSurface::Ptr surface = nullptr;
		HJ_CatchMapGetVal(i_param, HJOGEGLSurface::Ptr, surface);
		if (!surface)
		{
			i_err = -1;
			break;
		}
		if (renderModeIsContain(surface->getSurfaceType()))
		{
			std::vector<HJTransferRenderModeInfo::Ptr>& renderModes = renderModeGet(surface->getSurfaceType());
			for (auto it = renderModes.begin(); it != renderModes.end(); it++)
			{
				HJTransferRenderViewPortInfo::Ptr viewpotInfo = HJTransferRenderModeInfo::compute((*it), surface->getTargetWidth(), surface->getTargetHeight());
#if defined(HarmonyOS)       
				glViewport(viewpotInfo->x, viewpotInfo->y, viewpotInfo->width, viewpotInfo->height);
				i_err = m_draw->draw(m_LastFboInfo->m_texture, (*it)->cropMode, m_LastFboInfo->m_width, m_LastFboInfo->m_height, viewpotInfo->width, viewpotInfo->height, m_LastFboInfo->m_matrix, false);
#endif
				if (i_err < 0)
				{
					break;
				}
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

int HJPrioComSourceSeries::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
        HJ_CatchMapPlainGetVal(i_param, int, "InsIdx", m_insIdx);
		i_err = HJPrioComSourceBridge::init(i_param);
		if (i_err < 0)
		{
			break;
		}

        m_fbo->setInsName(HJFMT("{}_{}", m_fbo->getInsName(), m_insIdx));
        
		m_graph = HJPrioGraph::Create();
		i_err = m_graph->init(i_param);
		if (i_err < 0)
		{
			break;
		}
		
#if defined(HarmonyOS)        
		m_draw = HJOGCopyShaderStrip::Create();
        m_draw->setInsName(HJFMT("shader_{}_{}", m_draw->getInsName(), m_insIdx));
		i_err = m_draw->init(OGCopyShaderStripFlag_2D);
		if (i_err < 0)
		{
			HJFLogi("Draw init i_err:{}", i_err);
			break;
		}
#endif
	} while (false);
	return i_err;
}
void HJPrioComSourceSeries::done()
{
    if (m_graph)
    {
        m_graph->done();
        m_graph = nullptr;
    }    
    if (m_fbo)
    {
        m_fbo->done();
        m_fbo = nullptr;
    }    
	HJPrioComSourceBridge::done();
}

NS_HJ_END