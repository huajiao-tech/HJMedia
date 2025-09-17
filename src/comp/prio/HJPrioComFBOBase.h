#pragma once

#include "HJPrioCom.h"

#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

NS_HJ_BEGIN

class HJOGFBOCtrl;

#if defined(HarmonyOS)
class HJPrioComBaseFBOInfo
{
public:
    HJ_DEFINE_CREATE(HJPrioComBaseFBOInfo);
    HJPrioComBaseFBOInfo()
    {
        
    }
    HJPrioComBaseFBOInfo(int i_width, int i_height, float *i_matrix, GLuint i_texture):
         m_width(i_width)
        ,m_height(i_height)
        ,m_texture(i_texture)
    {
        if (i_matrix)
        {
            memcpy(m_matrix, i_matrix, sizeof(m_matrix));
        }
    }
    virtual ~HJPrioComBaseFBOInfo()
    {
        
    }
    //static HJPrioComBaseFBOInfo::Ptr Create(int i_width, int i_height, float *i_matrix, GLuint i_texture)
    //{
    //    return std::make_shared<HJPrioComBaseFBOInfo>(i_width, i_height, i_matrix, i_texture);
    //}
    
    int m_width  = 0;
    int m_height = 0;
    float m_matrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; 
    
    GLuint m_texture = 0;
};
#endif

class HJPrioComFBOBase : public HJPrioCom
{
public:
    HJ_DEFINE_CREATE(HJPrioComFBOBase);
    HJPrioComFBOBase();
    virtual ~HJPrioComFBOBase();
        
//	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
    
    int width();
    int height();
#if defined(HarmonyOS)    
    GLuint texture();
#endif
    float *getMatrix();
    
    void check(int i_width, int i_height, bool i_bTransparency = true);
    bool IsReady()
    {
        return m_bReady;
    }
    int draw(std::function<int()> i_func);
private:
    std::shared_ptr<HJOGFBOCtrl> m_fbo = nullptr;
    bool m_bReady = false;
};

NS_HJ_END



