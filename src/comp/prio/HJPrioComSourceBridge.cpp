#include "HJPrioComSourceBridge.h"
#include "HJPrioComSourceSplitScreen.h"
#include "HJPrioComSourceSeries.h"
#include "HJFLog.h"
#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGEGLSurface.h"
#endif

NS_HJ_BEGIN

HJPrioComSourceBridge::HJPrioComSourceBridge()
{
    HJ_SetInsName(HJPrioComSourceBridge);
    setPriority(HJPrioComType_VideoBridgeSrc);
}
HJPrioComSourceBridge::~HJPrioComSourceBridge()
{
    HJFLogi("~{}", getInsName());
}
#if defined(HarmonyOS)
HJOGRenderWindowBridge::Ptr HJPrioComSourceBridge::renderWindowBridgeAcquire()
{
    int i_err = 0;
    do 
    {
        if (!m_bridge)
        {
            m_bridge = HJOGRenderWindowBridge::Create();
            m_bridge->setInsName(m_insName + "_bridge");
            HJFLogi("{} renderThread renderWindowBridgeAcquire enter", getInsName());
            i_err = m_bridge->init();
            if (i_err < 0)
            {
                break;
            } 
        }
    } while (false);
    return m_bridge;   
}
std::shared_ptr<HJOGRenderWindowBridge> HJPrioComSourceBridge::renderWindowBridgeAcquireSoft()
{
    int i_err = 0;
    do 
    {
        if (!m_softBridge)
        {
            m_softBridge = HJOGRenderWindowBridge::Create();
            m_softBridge->setInsName(m_insName + "_bridge_soft");
            HJFLogi("{} softbridge renderWindowBridgeAcquireSoft enter", m_insName);
            i_err = m_softBridge->init();
            if (i_err < 0)
            {
                break;
            } 
        }
    } while (false);
    return m_softBridge;       
}
void HJPrioComSourceBridge::renderWindowBridgeReleaseSoft()
{
    priReleaseSoftBridge();
}
void HJPrioComSourceBridge::priReleaseSoftBridge()
{
    if (m_softBridge)
    {
        HJFLogi("{} softbridge done", getInsName());
#if defined(HarmonyOS)
        m_softBridge->done();
#endif
        m_softBridge = nullptr;
    }
}
#endif

void HJPrioComSourceBridge::stat()
{
    
    const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
    if (bridge == m_softBridge)
    {
        HJFLogi("{} stat use soft bridge idx:{}", getInsName(), m_statIdx);
    }   
    else
    {
        if (m_statIdx < 5)
        {
            HJFLogi("{} stat use main bridge idx:{}", getInsName(), m_statIdx);
        }
    }
    m_statIdx++;
}

int HJPrioComSourceBridge::update(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        if (!IsStateAvaiable())
        {
            break;
        }
        
#if defined(HarmonyOS)
        if (priIsMainAvaiable())
        {
            i_err = m_bridge->update();
            if (i_err < 0)
            {
                break;
            }
        }
        
        if (priIsSoftAvaiable())
        {
            i_err = m_softBridge->update();
            //HJFLogi("{} softbridge m_softBridge update i_err:{}", m_insName, i_err);
            if (i_err < 0)
            {
                break;
            }
        }
 #endif           
        
    } while (false);
    return i_err;
}

int HJPrioComSourceBridge::render(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        if (!IsStateReady())
        {
            break;
        }
        i_err = priRender(i_param, getBridge());
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
int HJPrioComSourceBridge::priRender(const HJBaseParam::Ptr& i_param, const std::shared_ptr<HJOGRenderWindowBridge> & i_bridge)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
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
                    i_err = i_bridge->draw(*it, surface->getTargetWidth(), surface->getTargetHeight());
                    if (i_err < 0)
                    {
                        break;
                    }
                }
                if (i_err < 0)
                {
                    break;
                }
                stat();
            }
#endif        
    } while (false);
    return i_err;
}
int HJPrioComSourceBridge::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        HJFLogi("{} init enter", getInsName());
        
        if (i_param->contains("renderListener"))
        {
            m_renderListener = i_param->getValue<HJListener>("renderListener");
        }                  
        
        i_err = HJPrioCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }
   
    } while (false);
    HJFLogi("{} init end i_err:{}", getInsName(), i_err);
    return i_err;
}
bool HJPrioComSourceBridge::priIsMainAvaiable()
{
    bool bAvaiable = false;
    do
    {
#if defined(HarmonyOS)  
        if (m_bridge && m_bridge->IsStateAvaiable())
        {
            bAvaiable = true;
            break;
        } 
#endif
    } while (false);
    return bAvaiable;    
}
bool HJPrioComSourceBridge::priIsSoftAvaiable()
{
    bool bAvaiable = false;
    do
    {
#if defined(HarmonyOS)  
        if (m_softBridge && m_softBridge->IsStateAvaiable())
        {
            bAvaiable = true;
            break;
        } 
#endif
    } while (false);
    return bAvaiable;    
}
bool HJPrioComSourceBridge::priIsMainReady()
{
    bool bReady = false;
    do
    {
#if defined(HarmonyOS)  
        if (m_bridge && m_bridge->IsStateReady())
        {
            bReady = true;
            break;
        } 
#endif
    } while (false);
    return bReady;
}
bool HJPrioComSourceBridge::priIsSoftReady()
{
    bool bReady = false;
    do
    {
#if defined(HarmonyOS)  
        if (m_softBridge && m_softBridge->IsStateReady())
        {
            bReady = true;
            break;
        } 
#endif
    } while (false);
    return bReady;    
}

bool HJPrioComSourceBridge::IsStateAvaiable()
{
    bool bAvaiable = false;
    do
    {
#if defined(HarmonyOS)        
        if (priIsMainAvaiable())
        {
            bAvaiable = true;
            break;
        }    
        
        if (priIsSoftAvaiable())
        {
            bAvaiable = true;
            break;
        }
#endif        
    } while (false);
    return bAvaiable;    
}
bool HJPrioComSourceBridge::IsStateReady()
{
    bool bReady = false;
    do
    {
#if defined(HarmonyOS)        
        if (priIsMainReady())
        {
            bReady = true;
            break;
        }    
        
        if (priIsSoftReady())
        {
            bReady = true;
            break;
        }
#endif        
    } while (false);
    return bReady;
}
int HJPrioComSourceBridge::getWidth()
{
    int width = 0;
    do {
#if defined(HarmonyOS)    
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            width = bridge->width();
        }
#endif
    } while (false);
    return width;
}
int HJPrioComSourceBridge::getHeight()
{
    int height = 0;
    do {
#if defined(HarmonyOS)  
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            height = bridge->height();
        }
#endif
    } while (false);
    return height;    
}
float * HJPrioComSourceBridge::getTexMatrix()
{
    float *matrix = nullptr;
    do {
#if defined(HarmonyOS)  
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            matrix = bridge->getTexMatrix();
        }
#endif
    } while (false);
    return matrix;     
}
#if defined (HarmonyOS)
GLuint HJPrioComSourceBridge::getTextureId()
{
    GLuint texture = 0;
    do {
#if defined(HarmonyOS)  
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            texture = bridge->getTextureId();
        }    
#endif
    } while (false);
    return texture;     
}
#endif

const std::shared_ptr<HJOGRenderWindowBridge>& HJPrioComSourceBridge::getBridge()
{
    if (priIsMainReady())
    {
        return m_bridge;
    }    
    if (priIsSoftReady())
    {
        return m_softBridge;
    }    
    return m_nullBridge;
}

void HJPrioComSourceBridge::done() 
{
    if (m_bridge)
    {
#if defined(HarmonyOS)
        m_bridge->done();
#endif
        m_bridge = nullptr;
    }
    priReleaseSoftBridge();
    
    HJPrioCom::done();
}

HJPrioComSourceBridge::Ptr HJPrioComSourceBridge::CreateFactory(HJPrioComSourceType i_type)
{
    HJPrioComSourceBridge::Ptr val = nullptr;
    switch (i_type)
    {
    case HJPrioComSourceType_Bridge:
        val = HJPrioComSourceBridge::Create();
        HJFLogi("source type bridge");
        break;
    case HJPrioComSourceType_SPLITSCREEN:
        val = HJPrioComSourceSplitScreen::Create();
        HJFLogi("source type splice screen");
        break;
    case HJPrioComSourceType_SERIES:
        val = HJPrioComSourceSeries::Create();
        HJFLogi("source type series");        
        break;
    default:
        break;
    }
    return val;
}

NS_HJ_END