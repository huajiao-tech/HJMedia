#if defined(HarmonyOS)
#include "HJPrioComFBOGray.h"
#include "HJFLog.h"
#include "HJOGFBOCtrl.h"
#include "HJOGCopyShaderStrip.h"
#include "HJOGShaderCommon.h"


NS_HJ_BEGIN

class HJOGCopyShaderStripGray : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGCopyShaderStripGray);
    HJOGCopyShaderStripGray()
    {}
    virtual ~HJOGCopyShaderStripGray()
    {}
    virtual std::string shaderGetFragment() override 
    {
        return s_fragmentShaderGray;
    }
private:
   const static std::string s_fragmentShaderGray;
};

const std::string HJOGCopyShaderStripGray::s_fragmentShaderGray = R"(
precision mediump float;
varying vec2  v_texcood; 
uniform sampler2D sTexture;
void main()
{
    vec4 color = texture2D(sTexture,v_texcood);
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    gl_FragColor = vec4(vec3(gray), color.a);    
}
)";
//////////////////////////////////////////


HJPrioComFBOGray::HJPrioComFBOGray()
{
    HJ_SetInsName(HJPrioComFBOGray);
    setPriority(HJPrioComType_VideoGray);
}

HJPrioComFBOGray::~HJPrioComFBOGray()
{

}

int HJPrioComFBOGray::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        HJPrioComBaseFBOInfo::Ptr info = nullptr;
        HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
        if (!info)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }    
        HJPrioComFBOBase::check(info->m_width, info->m_height);
#endif
    } while (false);
    return i_err;
}
int HJPrioComFBOGray::render(HJBaseParam::Ptr i_param) 
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
        if (m_draw)
        {
            HJPrioComBaseFBOInfo::Ptr info = nullptr;
            HJ_CatchMapGetVal(i_param, HJPrioComBaseFBOInfo::Ptr, info);
            if (!info)
            {
                i_err = HJ_WOULD_BLOCK;
                break;
            } 
            i_err = HJPrioComFBOBase::draw([this, info]()
            {
                return m_draw->draw(info->m_texture, "crop", info->m_width, info->m_height, info->m_width, info->m_height, info->m_matrix, false);
            });
        }
#endif
    } while (false);
    return i_err;
}

int HJPrioComFBOGray::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        i_err = HJPrioComFBOBase::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)       
        m_draw = HJOGCopyShaderStripGray::Create();
        i_err = m_draw->init(OGCopyShaderStripFlag_2D);
        if (i_err < 0)
        {
            break;
        }
#endif
    } while (false);
    return i_err;
}
void HJPrioComFBOGray::done() 
{
#if defined(HarmonyOS)
    if (m_draw)
    {
        m_draw->release();
        m_draw = nullptr;
    }    
#endif
    HJPrioComFBOBase::done();
}

NS_HJ_END

#endif