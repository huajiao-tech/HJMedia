#include "HJPrioComFBOBase.h"
#include "HJFLog.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGRenderEnv.h"
#include "HJOGFBOCtrl.h"
#endif

NS_HJ_BEGIN

HJPrioComFBOBase::HJPrioComFBOBase()
{
    HJ_SetInsName(HJPrioComFBOBase);
    setPriority(HJPrioComType_None);
}
HJPrioComFBOBase::~HJPrioComFBOBase()
{
    HJFLogi("{} ~HJPrioComFBOBase", getInsName());
}
int HJPrioComFBOBase::width()
{
#if defined(HarmonyOS) 
    if (m_fbo)
    {
        return m_fbo->getWidth();
    }
#endif
    return 0;    
}
int HJPrioComFBOBase::height()
{
#if defined(HarmonyOS) 
    if (m_fbo)
    {
        return m_fbo->getHeight();
    }
#endif
    return 0;
}
float *HJPrioComFBOBase::getMatrix()
{
#if defined(HarmonyOS) 
    if (m_fbo)
    {
        return m_fbo->getMatrix();
    }
#endif
    return nullptr;
}

#if defined(HarmonyOS)    
GLuint HJPrioComFBOBase::texture()
{
    if (m_fbo)
    {
        return m_fbo->getTextureId();
    }
    return 0;    
}
#endif

int HJPrioComFBOBase::update(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        
    } while (false);
    return i_err;
}
int HJPrioComFBOBase::render(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        
    } while (false);
    return i_err;
}
void HJPrioComFBOBase::check(int i_width, int i_height, bool i_bTransparency)
{
#if defined(HarmonyOS) 
    if ((i_width > 0) && (i_height > 0))
    {
        bool bResetFbo = false;
        if (!m_fbo)
        {
            bResetFbo = true;
        }   
        else 
        {
            if ((i_width != m_fbo->getWidth()) || (i_height != m_fbo->getHeight()))
            {
                bResetFbo = true;
            }
        }   

        if (bResetFbo)
        {
            m_fbo = HJOGFBOCtrl::Create();
            HJFLogi("{} fbo change width:{} height:{}", getInsName(), i_width, i_height);
            m_fbo->init(i_width, i_height, i_bTransparency);
            m_bReady = true;
        }
    }    
#endif
}
int HJPrioComFBOBase::draw(std::function<int()> i_func)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS) 
        if (m_fbo)
        {
            m_fbo->attach();
            i_err = i_func();
            if (i_err < 0)
            {
                m_fbo->detach();
                break;
            }
            m_fbo->detach();
        } 
        else 
        {
            i_err = HJ_WOULD_BLOCK;
        }
#endif
    } while (false);
    return i_err;
}
//int HJPrioComFBOBase::onDraw(HJBaseParam::Ptr i_param)
//{
//    return HJ_OK;
//}

void HJPrioComFBOBase::done()
{
#if defined(HarmonyOS)
    if (m_fbo)
    {
        m_fbo->done();
        m_fbo = nullptr;
    }    
#endif
    HJPrioCom::done();
}

NS_HJ_END