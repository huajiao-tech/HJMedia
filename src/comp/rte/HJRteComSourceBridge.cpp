#include "HJRteComSource.h"
#include "HJFLog.h"
#include "HJRteGraphBaseEGL.h"
#include "HJOGEGLSurface.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#endif

NS_HJ_BEGIN

///////////////////////////////////////////////////
void HJRteComSource::done()
{
    if (m_dynamicFbo)
    {
        HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(m_dynamicFbo);
        m_dynamicFbo = nullptr;
    }
    HJRteCom::done();
}
//////////////////////////////////////////////////

HJRteComSourceBridge::HJRteComSourceBridge()
{
    HJ_SetInsName(HJRteComSourceBridge);
    HJRteCom::setPriority(HJRteComPriority_VideoSource);
    m_textureType = HJRteTextureType_OES;
}
HJRteComSourceBridge::~HJRteComSourceBridge()
{
    HJFLogi("{} ~HJRteComSourceBridge", getDebugName());
}
#if defined(HarmonyOS)
HJOGRenderWindowBridge::Ptr HJRteComSourceBridge::renderWindowBridgeAcquire()
{
    int i_err = 0;
    do 
    {
        if (!m_bridge)
        {
            m_bridge = HJOGRenderWindowBridge::Create();
            m_bridge->setInsName(m_insName + "_bridge_" + std::to_string(getDebugIdx()));
            HJFLogi("{} renderThread renderWindowBridgeAcquire enter", m_insName);
            i_err = m_bridge->init();
            if (i_err < 0)
            {
                break;
            } 
        }
    } while (false);
    return m_bridge;   
}
std::shared_ptr<HJOGRenderWindowBridge> HJRteComSourceBridge::renderWindowBridgeAcquireSoft()
{
    int i_err = 0;
    do 
    {
        if (!m_softBridge)
        {
            m_softBridge = HJOGRenderWindowBridge::Create();
            m_softBridge->setInsName(m_insName + "_bridge_soft_" + std::to_string(getDebugIdx()));
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
void HJRteComSourceBridge::renderWindowBridgeReleaseSoft()
{
    priReleaseSoftBridge();
}
void HJRteComSourceBridge::priReleaseSoftBridge()
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

void HJRteComSourceBridge::stat()
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

int HJRteComSourceBridge::update(HJBaseParam::Ptr i_param)
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

//int HJRteComSourceBridge::render(HJBaseParam::Ptr i_param)
//{
//    int i_err = 0;
//    do
//    {
//        if (!IsStateReady())
//        {
//            break;
//        }
//        i_err = priRender(i_param, getBridge());
//        if (i_err < 0)
//        {
//            break;
//        }
//    } while (false);
//    return i_err;
//}
int HJRteComSourceBridge::priRender(const HJBaseParam::Ptr& i_param, const std::shared_ptr<HJOGRenderWindowBridge> & i_bridge)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        
        int targetWidth, targetHeight = 0;
        HJ_CatchMapPlainGetVal(i_param, int, "targetWidth", targetWidth);
        HJ_CatchMapPlainGetVal(i_param, int, "targetHeight", targetHeight);
        HJTransferRenderModeInfo::Ptr info;
        HJ_CatchMapGetVal(i_param, HJTransferRenderModeInfo::Ptr, info);
        
        i_err = i_bridge->draw(info, targetWidth, targetHeight);
        if (i_err < 0)
        {
            break;
        }
#endif        
    } while (false);
    return i_err;
}
int HJRteComSourceBridge::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJRteCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }
   
    } while (false);
    return i_err;
}
bool HJRteComSourceBridge::priIsMainAvaiable()
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
bool HJRteComSourceBridge::priIsSoftAvaiable()
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
bool HJRteComSourceBridge::priIsMainReady()
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
bool HJRteComSourceBridge::priIsSoftReady()
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

bool HJRteComSourceBridge::IsStateAvaiable()
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

bool HJRteComSourceBridge::IsStateReady()
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
int HJRteComSourceBridge::getWidth()
{
    do {
#if defined(HarmonyOS)    
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            m_width = bridge->width();
        }
#endif
    } while (false);
    return m_width;
}
int HJRteComSourceBridge::getHeight()
{
    do {
#if defined(HarmonyOS)  
        const std::shared_ptr<HJOGRenderWindowBridge>& bridge = getBridge();
        if (bridge)
        {
            m_height = bridge->height();
        }
#endif
    } while (false);
    return m_height;
}

float * HJRteComSourceBridge::getTexMatrix()
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

GLuint HJRteComSourceBridge::getTextureId()
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

const std::shared_ptr<HJOGRenderWindowBridge>& HJRteComSourceBridge::getBridge()
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

void HJRteComSourceBridge::done() 
{
#if defined(HarmonyOS)
    if (m_bridge)
    {
        m_bridge->done();
        m_bridge = nullptr;
    }
    priReleaseSoftBridge();
#endif   
    HJRteCom::done();
}

HJRteComSourceBridge::Ptr HJRteComSourceBridge::CreateFactory()
{
    return HJRteComSourceBridge::Create();
}


////////////////////////////////////////////////////////////
HJRteComSourceSplitScreen::HJRteComSourceSplitScreen()
{
    HJ_SetInsName(HJRteComSourceSplitScreen);
    HJRteCom::setPriority(HJRteComPriority_VideoSource);
    m_textureType = HJRteTextureType_OES;
}
HJRteComSourceSplitScreen::~HJRteComSourceSplitScreen()
{

}

int HJRteComSourceSplitScreen::getWidth() 
{
    return HJRteComSourceBridge::getWidth() / 2;
}

////////////////////////////////////////////////
HJRteComSourceSplitScreenMediaData::HJRteComSourceSplitScreenMediaData()
{
	HJ_SetInsName(HJRteComSourceSplitScreenMediaData);
	HJRteCom::setPriority(HJRteComPriority_VideoSource);
	m_textureType = HJRteTextureType_2D;
}
int HJRteComSourceSplitScreenMediaData::getWidth()
{
    return HJRteComSourceBridgeMediaData::getWidth() / 2;
}
NS_HJ_END