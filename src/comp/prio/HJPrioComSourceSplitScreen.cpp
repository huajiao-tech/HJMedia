#include "HJPrioComSourceSplitScreen.h"
#include "HJFLog.h"

#if defined(HarmonyOS)
#include "HJOGRenderWindowBridge.h"
#include "HJOGBaseShader.h"
#include "HJOGEGLSurface.h"
#endif

NS_HJ_BEGIN

#if defined(HarmonyOS)
class HJOGCopyShaderStripSplitScreenLR : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGCopyShaderStripSplitScreenLR);
    HJOGCopyShaderStripSplitScreenLR()
    {}
    virtual ~HJOGCopyShaderStripSplitScreenLR()
    {}
    virtual std::string shaderGetFragment() override 
    {
        return s_shaderSplitScreenLR_OES;
    }
private:
   const static std::string s_shaderSplitScreenLR_OES;
};

const std::string HJOGCopyShaderStripSplitScreenLR::s_shaderSplitScreenLR_OES = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2  v_texcood;    
uniform samplerExternalOES sTexture;
void main() 
{
    vec4 planeAlpha = texture2D(sTexture,vec2(v_texcood.x * 0.5, v_texcood.y));
    vec4 planeColor = texture2D(sTexture,vec2(v_texcood.x * 0.5 + 0.5, v_texcood.y));
    gl_FragColor = vec4(planeColor.rgb * planeAlpha.r, planeAlpha.r);    
}
)";
#endif
////////////////////////////////////////////////////////////////////////////////
HJPrioComSourceSplitScreen::HJPrioComSourceSplitScreen()
{
    HJ_SetInsName(HJPrioComSourceSplitScreen);
}
HJPrioComSourceSplitScreen::~HJPrioComSourceSplitScreen()
{
    HJFLogi("~{}", getInsName());
}

int HJPrioComSourceSplitScreen::render(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)
        if (HJPrioComSourceBridge::IsStateReady())
        {
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
      
				    glViewport(viewpotInfo->x, viewpotInfo->y, viewpotInfo->width, viewpotInfo->height);
                    
                    int srcWidth = HJPrioComSourceBridge::getWidth();
                    int srcHeight = HJPrioComSourceBridge::getHeight();
				    i_err = m_draw->draw(HJPrioComSourceBridge::getTextureId(), (*it)->cropMode, srcWidth / 2, srcHeight, viewpotInfo->width, viewpotInfo->height, HJPrioComSourceBridge::getTexMatrix(), false, m_bMirror);
                    if (i_err < 0)
                    {
                        break;
                    }
                    HJPrioComSourceBridge::stat();
                }
                if (i_err < 0)
                {
                    break;
                }
            }

        }
#endif
    } while (false);
    return i_err;
}

int HJPrioComSourceSplitScreen::init(HJBaseParam::Ptr i_param)
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
        m_draw = HJOGCopyShaderStripSplitScreenLR::Create();
        i_err = m_draw->init(OGCopyShaderStripFlag_OES);
        if (i_err < 0)
        {
            break;
        }
#endif
        HJ_CatchMapPlainGetVal(i_param, bool, "IsMirror", m_bMirror);
    } while (false);
    return i_err;
}
void HJPrioComSourceSplitScreen::done() 
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