#include "HJPrioComFBOBridgeSrcEx.h"
#include "HJFLog.h"
#include "HJPrioComFBOBase.h"
#include "HJTransferInfo.h"
#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGFBOCtrl.h"
#include "HJOGBaseShader.h"
#include "HJOGShaderCommon.h"
#endif

NS_HJ_BEGIN

HJPrioComFBOBridgeSrcEx::HJPrioComFBOBridgeSrcEx()
{
    HJ_SetInsName(HJPrioComFBOBridgeSrcEx);
    m_fbo = HJPrioComFBOBase::Create();
}

HJPrioComFBOBridgeSrcEx::~HJPrioComFBOBridgeSrcEx()
{
    HJFLogi("~HJPrioComFBOBridgeSrcEx");
}

int HJPrioComFBOBridgeSrcEx::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        if (!HJPrioComSourceBridge::IsStateAvaiable())
        {
            break;
        }    
        
        i_err = HJPrioComSourceBridge::update(i_param);
        if (i_err < 0)
        {
            break;
        }    
        
        int width = HJPrioComSourceBridge::getWidth();
        int height = HJPrioComSourceBridge::getHeight();
        m_fbo->check(width, height);
#endif        
    } while (false);
    return i_err;
}

int HJPrioComFBOBridgeSrcEx::priRender()
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        i_err = m_draw->draw(HJPrioComSourceBridge::getTextureId(), "clip", m_fbo->width(), m_fbo->height(), m_fbo->width(), m_fbo->height(), HJPrioComSourceBridge::getTexMatrix());
#endif    
    } while (false);
    return i_err; 
}
int HJPrioComFBOBridgeSrcEx::render(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        if (!HJPrioComSourceBridge::IsStateReady())
        {
            break;
        }
        
#if defined(HarmonyOS)  
        i_err = m_fbo->draw([this]()
        {
            return priRender();
        });
#endif
    } while (false);
    return i_err;
}
#if defined(HarmonyOS)
int HJPrioComFBOBridgeSrcEx::width()
{
    return HJPrioComSourceBridge::getWidth();
}
int HJPrioComFBOBridgeSrcEx::height()
{
    return HJPrioComSourceBridge::getHeight();
}
float *HJPrioComFBOBridgeSrcEx::getMatrix()
{
    return m_fbo->getMatrix();
}
GLuint HJPrioComFBOBridgeSrcEx::texture()
{
    return m_fbo->texture();
}
#endif
int HJPrioComFBOBridgeSrcEx::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJPrioComSourceBridge::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)
        m_draw = HJOGCopyShaderStrip::Create();
        i_err = m_draw->init(OGCopyShaderStripFlag_OES);
        if (i_err < 0)
        {
            break;
        }    
#endif
    } while (false);
    return i_err;
}
void HJPrioComFBOBridgeSrcEx::done() 
{
    if (m_draw)
    {
#if defined(HarmonyOS)
        m_draw->release();
#endif
        m_draw = nullptr;
    }    
    HJPrioComSourceBridge::done();
}

NS_HJ_END