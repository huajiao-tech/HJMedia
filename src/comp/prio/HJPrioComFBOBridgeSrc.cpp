#include "HJPrioComFBOBridgeSrc.h"
#include "HJFLog.h"
#include "HJTransferInfo.h"
#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGRenderEnv.h"
#include "HJOGFBOCtrl.h"
#include "HJOGCopyShaderStrip.h"
#include "HJOGShaderCommon.h"
#endif

NS_HJ_BEGIN

HJPrioComFBOBridgeSrc::HJPrioComFBOBridgeSrc()
{
    HJ_SetInsName(HJPrioComFBOBridgeSrc);
    setPriority(HJPrioComType_VideoBridgeSrc);
}

HJPrioComFBOBridgeSrc::~HJPrioComFBOBridgeSrc()
{
    HJFLogi("~HJPrioComBridgeSrcFBO");
}
#if defined(HarmonyOS)
std::shared_ptr<HJOGRenderWindowBridge> HJPrioComFBOBridgeSrc::renderWindowBridgeAcquire()
{
    int i_err = 0;
    do 
    {
        if (!m_bridge)
        {
            m_bridge = HJOGRenderWindowBridge::Create();
            m_bridge->setInsName(m_insName + "_bridge");
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
#endif

int HJPrioComFBOBridgeSrc::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        if (m_bridge)
        {
            i_err = m_bridge->update();
            if (i_err != HJ_OK)
            {
                break;
            }
            HJPrioComFBOBase::check(m_bridge->width(), m_bridge->height());
        }
#endif        
    } while (false);
    return i_err;
}
int HJPrioComFBOBridgeSrc::render(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        if (!HJPrioComFBOBase::IsReady())
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)  
        i_err = HJPrioComFBOBase::draw([this]()
        {
            return m_bridge->draw(HJTransferRenderModeInfo::Create(), HJPrioComFBOBase::width(), HJPrioComFBOBase::height());
        });
#endif
    } while (false);
    return i_err;
}

int HJPrioComFBOBridgeSrc::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJPrioComFBOBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJPrioComFBOBridgeSrc::done() 
{
    HJPrioComFBOBase::done();
}

NS_HJ_END