#include "HJComVideoCapture.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJComVideoCapture::HJComVideoCapture()
{
    HJ_SetInsName(HJComVideoCapture);
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);
}
HJComVideoCapture::~HJComVideoCapture()
{
    HJFLogi("~HJComVideoCapture");
}

HJOGRenderWindowBridge::Ptr HJComVideoCapture::renderWindowBridgeAcquire()
{
    return m_bridge;
}
int HJComVideoCapture::doRenderUpdate()
{
    int i_err = 0;
    do 
    {
        if (m_bridge)
        {
            i_err = m_bridge->update();
            if (i_err < 0)
            {
                break;
            }    
        }    
        i_err = renderUpdate();
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
int HJComVideoCapture::doRenderDraw(int i_targetWidth, int i_targetHeight)
{
    int i_err = 0;
    do 
    {
        if (m_bridge)
        {
            i_err = m_bridge->draw(i_targetWidth, i_targetHeight);
            if (i_err < 0)
            {
                break;
            } 
        }    
        i_err = renderDraw(i_targetWidth, i_targetHeight);
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}

int HJComVideoCapture::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJBaseRenderCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        
        std::string renderMode = "";
        if (i_param && i_param->contains(HJBaseParam::s_paramRenderBridge))
        {
            renderMode = i_param->getValue<std::string>(HJBaseParam::s_paramRenderBridge);
        }
        if (renderMode.empty())
        {
            i_err = -1;
            break;
        }    
        
        m_bridge = HJOGRenderWindowBridge::Create();
        m_bridge->setInsName(m_insName + "_bridge");
        HJFLogi("{} renderThread renderWindowBridgeAcquire enter", m_insName);
        i_err = m_bridge->init(renderMode);
    
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err;
}
void HJComVideoCapture::done() 
{
    if (m_bridge)
    {
        m_bridge->done();
        m_bridge = nullptr;
    }
    HJBaseRenderCom::done();
}

NS_HJ_END