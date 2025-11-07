#include "HJRteComDraw.h"
#include "HJFLog.h"

#include "HJRteUtils.h"
#if defined(HarmonyOS)
#include "HJOGBaseShader.h"
#include "HJOGEGLSurface.h"
#include "HJOGShaderCommon.h"
#include "HJOGFBOCtrl.h"
#endif

NS_HJ_BEGIN

HJRteComDraw::HJRteComDraw()
{
    
}
HJRteComDraw::~HJRteComDraw()
{
    
}
std::string HJRteComDraw::mapWindowRenderMode(HJRteWindowRenderMode i_windowRenderMode)
{
#if defined(HarmonyOS)
    std::string renderMode = HJOGShaderCommon::s_render_mode_clip;
    switch(i_windowRenderMode)
    {
    case HJRteWindowRenderMode_CLIP:
        renderMode = HJOGShaderCommon::s_render_mode_clip;
        break;
    case HJRteWindowRenderMode_FIT:
        renderMode = HJOGShaderCommon::s_render_mode_fit;
        break;
    case HJRteWindowRenderMode_FULL:
        renderMode = HJOGShaderCommon::s_render_mode_full;
        break;
    case HJRteWindowRenderMode_ORIGIN:
        renderMode = HJOGShaderCommon::s_render_mode_origin;
        break;
    default:
        break;
    }
    return renderMode;
#else
    return "";
#endif
}
int HJRteComDraw::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
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

void HJRteComDraw::done()
{
    HJRteCom::done();
}

int HJRteComDraw::render(HJRteDriftInfo::Ptr& o_param)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)
        i_err = foreachPreLink([this, &o_param](const std::shared_ptr<HJRteComLink> &i_link)
        {
            int ret = HJ_OK;
            do
            { 
                std::string linkName = i_link->getInsName();
                HJFLogi("{} render linkName:{}", getInsName(), linkName);
                const std::shared_ptr<HJRteComLinkInfo>& linkInfo = i_link->getLinkInfo();
                if (!linkInfo)
                {
                    ret = HJErrFatal;
                    break;
                }  
                
                HJRteDriftInfo::Ptr driftInfo = i_link->getDriftInfo();
                if (!driftInfo)
                {
                    ret = HJErrFatal;
                    break;
                }    
                
                if (m_customDraw)
                {
                    ret = m_customDraw(driftInfo, o_param);
                    if (ret < 0)
                    {
                        break;
                    }
                } 
                else  
                {
                    HJRteComDrawFBO::Ptr fboPtr = HJ_CvtDynamic(HJRteComDrawFBO, HJBaseSharedObject::getSharedFrom(this));
                    if (fboPtr)
                    {
                        fboPtr->check(driftInfo->m_srcWidth, driftInfo->m_srcHeight);
                    }    
                    
                    ret = attach();
                    if (ret < 0)
                    {
                        break;
                    }
                    HJSizei sizei = getTargetWH();
                    HJVec4i veci = linkInfo->convert(sizei.w, sizei.h);
                    int viewOffx = veci.x;
                    int viewOffy = veci.y;
                    int viewTargetW = veci.z;
                    int viewTargetH = veci.w;
                    glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);
                       
                    HJOGBaseShader::Ptr shader = nullptr;
                    if (i_link->getShader())
                    {
                        shader = i_link->getShader();
                        HJFLogi("{} find shader, link have shader", getInsName());
                    }    
                    else if (fboPtr && fboPtr->getShader())
                    {
                        shader = fboPtr->getShader();
                        HJFLogi("{} find shader, fbo have shader", getInsName());
                    } 
                    else 
                    {
                        ret = HJErrFatal;
                        HJFLoge("{} not find shader, error", getInsName());
                        break;
                    }
                    ret = shader->draw(driftInfo->m_textureId, mapWindowRenderMode(driftInfo->m_windowRenderMode), driftInfo->m_srcWidth, driftInfo->m_srcHeight, viewTargetW, viewTargetH, driftInfo->m_textureMat);
                    if (ret < 0)
                    {
                        break;
                    }   
                    
                    ret = detach();
                    if (ret < 0)
                    {
                        break;    
                    }
                    
                    if (fboPtr && fboPtr->isFilter())
                    {
                        o_param = HJRteDriftInfo::Create<HJRteDriftInfo>(fboPtr->getTextureId(), HJRteTextureType_2D, HJRteWindowRenderMode_CLIP, sizei.w, sizei.h);
                    }
                }
//               HJFLogi("{} com:<{} - {}> info:{} render sub #########", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());

            } while(false);
            return ret;
        });
        if (i_err < 0)
        {
            break;
        }
#endif
    } while (false);
    return i_err;
}
    
int HJRteComDraw::attach()
{
    return HJ_OK;
}
int HJRteComDraw::detach()
{
    return HJ_OK;
}
HJSizei HJRteComDraw::getTargetWH()
{
    return HJSizei{0,0};
}
////////////////////////////////////////////////
HJRteComDrawEGL::HJRteComDrawEGL()
{
    HJ_SetInsName(HJRteComDrawEGL);
    HJRteCom::setPriority(HJRteComPriority_Target);
}
int HJRteComDrawEGL::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDraw::init(i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;    
}
void HJRteComDrawEGL::done() 
{
    HJRteComDraw::done();
}


int HJRteComDrawEGL::bindEx()
{
    int i_err = HJ_OK;
    do 
    {
#if defined (HarmonyOS)
        HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
        if (eglSurface)
        {
            i_err = eglSurface->procMakeCurrentCb()();
            if (i_err < 0)
            {
                HJFLoge("{} makeCurrent error", getInsName());
                break;
            }    
        }
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
#endif        
    } while (false);
    return i_err;   
}

int HJRteComDrawEGL::unbindEx()
{
    int i_err = HJ_OK;
    do 
    {
#if defined (HarmonyOS)
            HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
            if (eglSurface)
            {
                i_err = eglSurface->procSwapCb()();
                if (i_err < 0)
                {
                    HJFLoge("{} swap error", getInsName());
                    break;
                } 
            }
#endif
    } while (false);
    return i_err;
}
int HJRteComDrawEGL::renderEx(const std::shared_ptr<HJRteComLink>&i_link, HJRteDriftInfo::Ptr& o_driftInfo)
{
    int i_err = HJ_OK;
    do
    {
        HJRteDriftInfo::Ptr driftInfo = i_link->getDriftInfo();
        if (!driftInfo)
        {
            HJFLoge("{} not find driftInfo, error", getInsName());
            i_err = HJErrFatal;
            break;
        }
        const std::shared_ptr<HJOGBaseShader>& shader = i_link->getShader();
        if (!shader)
        {
            HJFLoge("{} not find shader, error", getInsName());
            i_err = HJErrFatal;
            break;
        }

        HJRteComLinkInfo::Ptr linkInfo = i_link->getLinkInfo();
        HJSizei sizei = getTargetWH();
        HJVec4i veci = linkInfo->convert(sizei.w, sizei.h);
        int viewOffx = veci.x;
        int viewOffy = veci.y;
        int viewTargetW = veci.z;
        int viewTargetH = veci.w;
#if defined (HarmonyOS)    
        glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);
#endif
        i_err = shader->draw(driftInfo->m_textureId, mapWindowRenderMode(driftInfo->m_windowRenderMode), driftInfo->m_srcWidth, driftInfo->m_srcHeight, viewTargetW, viewTargetH, driftInfo->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
        if (i_err < 0)
        {
            break;
        }   
        
    } while (false);
    return i_err;
}
int HJRteComDrawEGL::attach()
{
    int i_err = HJ_OK;
    do 
    {
#if defined (HarmonyOS)
        HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
        if (eglSurface)
        {
            i_err = eglSurface->procMakeCurrentCb()();
            if (i_err < 0)
            {
                HJFLoge("{} makeCurrent error", getInsName());
                break;
            }    
        }
        
        if (m_curRenderIdx == 0)
        {

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		    glClear(GL_COLOR_BUFFER_BIT);

        }
        m_curRenderIdx++;
#endif        
    } while (false);
    return i_err;
}
void HJRteComDrawEGL::reset()
{
    m_curRenderIdx = 0;
}
int HJRteComDrawEGL::detach()
{
    int i_err = HJ_OK;
    do {
        int nPreSize = m_preQueue.size();
        if (m_curRenderIdx == nPreSize)
        {
#if defined (HarmonyOS)
            HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
            if (eglSurface)
            {
                i_err = eglSurface->procSwapCb()();
                if (i_err < 0)
                {
                    HJFLoge("{} swap error", getInsName());
                    break;
                } 
            }
#endif
        }    
    } while (false);
    return i_err;
}

HJSizei HJRteComDrawEGL::getTargetWH() 
{
#if defined(HarmonyOS)
    HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
    if (eglSurface)
    {
        return HJSizei{eglSurface->getTargetWidth(), eglSurface->getTargetHeight()};
    }    
#endif
    return HJSizei{0,0};
}

#if defined(HarmonyOS)
void HJRteComDrawEGL::setSurface(const std::shared_ptr<HJOGEGLSurface> & i_eglSurface)
{
    m_eglSurfaceWtr = i_eglSurface;
}
#endif



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HJRteComDrawFBO::HJRteComDrawFBO()
{
    HJ_SetInsName(HJRteComDrawFBO);
    HJRteCom::setPriority(HJRteComPriority_Target);   
}

int HJRteComDrawFBO::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDraw::init(i_param);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;    
}
void HJRteComDrawFBO::done() 
{
#if defined(HarmonyOS)
    if (m_endFbo)
    {
        m_endFbo->done();
        m_endFbo = nullptr;
    }
#endif
    if (m_endShader)
    {
        m_endShader.reset();
        m_endShader = nullptr;
    }    
    HJRteComDraw::done();
}

HJSizei HJRteComDrawFBO::getTargetWH() 
{
#if defined(HarmonyOS)
    if (m_endFbo)
    {
        return HJSizei{m_endFbo->getWidth(), m_endFbo->getHeight()};
    }    
#endif
    return HJSizei{0, 0};
}



int HJRteComDrawFBO::check(int i_width, int i_height, bool i_bTransparency)
{
    int i_err = HJ_OK;
#if defined(HarmonyOS)
    if ((i_width > 0) && (i_height > 0))
    {
        bool bResetFbo = false;
        if (!m_endFbo)
        {
            bResetFbo = true;
        }   
        else 
        {
            if ((i_width != m_endFbo->getWidth()) || (i_height != m_endFbo->getHeight()))
            {
                bResetFbo = true;
            }
        }   

        if (bResetFbo)
        {
            m_endFbo = HJOGFBOCtrl::Create();
            HJFLogi("{} fbo change width:{} height:{}", getInsName(), i_width, i_height);
            i_err = m_endFbo->init(i_width, i_height, i_bTransparency);
        }
    }   
#endif
    return i_err;
}

#if defined(HarmonyOS)
GLuint HJRteComDrawFBO::getTextureId()
{
    if (m_endFbo)
    {
        return m_endFbo->getTextureId();
    }
    return 0;
}
#endif
int HJRteComDrawFBO::attach()
{
    int i_err = HJ_OK;
    do 
    {
        if (!m_endFbo)
        {
            i_err = HJErrFatal;
            break;
        }
#if defined(HarmonyOS)
        m_endFbo->attach();
#endif
    } while (false);
    return i_err;
}
int HJRteComDrawFBO::detach()
{
    int i_err = HJ_OK;
    do 
    {
        if (!m_endFbo)
        {
            i_err = HJErrFatal;
            break;
        }
#if defined(HarmonyOS)
        m_endFbo->detach();
#endif
    } while (false);
    return i_err;    
}

int HJRteComDrawFBO::bindEx()
{
    int i_err = HJ_OK;
    do 
    {
        m_isBinded = true;
        if (!m_endFbo)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        m_endFbo->attach();
#endif
    } while (false);
    return i_err; 
}
int HJRteComDrawFBO::unbindEx()
{
    int i_err = HJ_OK;
    do 
    {
        m_isBinded = false;
        if (!m_endFbo)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
#if defined(HarmonyOS)
        m_endFbo->detach();
#endif
    } while (false);
    return i_err;
}

int HJRteComDrawFBO::adjustResolution(int i_width, int i_height)
{
    int i_err = HJ_OK;
    do
    {
        if (!m_isAdjustResolution)
        {
            break;
        }
        i_err = HJRteComDrawFBO::tryRestartFbo(m_endFbo, i_width, i_height);
    } while (false);
    return i_err;
}
int HJRteComDrawFBO::tryRestartFbo(std::shared_ptr<HJOGFBOCtrl>& o_fbo, int i_width, int i_height, bool i_bTranspareny)
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)        
        bool bResetFbo = false;
        if (!o_fbo)
        {
            bResetFbo = true;
        }   
        else 
        {
            if ((i_width != o_fbo->getWidth()) || (i_height != o_fbo->getHeight()))
            {
                bResetFbo = true;
            }
        }   

        if (bResetFbo)
        {
            o_fbo = HJOGFBOCtrl::Create();
            i_err = o_fbo->init(i_width, i_height, i_bTranspareny);
            if (i_err < 0)
            {
                break;
            }
        }
#endif
    } while (false);
    return i_err; 
}
// int HJRteComDrawFBO::tryRestart(int i_width, int i_height, bool i_bTransparency)
// {
//     int i_err = HJ_OK;
// #if defined(HarmonyOS)
//     if ((i_width > 0) && (i_height > 0))
//     {
//         bool bResetFbo = false;
//         if (!m_endFbo)
//         {
//             bResetFbo = true;
//         }   
//         else 
//         {
//             if ((i_width != m_endFbo->getWidth()) || (i_height != m_endFbo->getHeight()))
//             {
//                 bResetFbo = true;
//             }
//         }   

//         if (bResetFbo)
//         {
//             if (m_isBinded)
//             {
//                 unbindEx();
//             }
//             m_endFbo = HJOGFBOCtrl::Create();
//             HJFLogi("{} fbo change width:{} height:{}", getInsName(), i_width, i_height);
//             i_err = m_endFbo->init(i_width, i_height, i_bTransparency);
//             bindEx();
//         }
//     }   
// #endif
//     return i_err;

// }
int HJRteComDrawFBO::renderEx(const std::shared_ptr<HJRteComLink>&i_link, HJRteDriftInfo::Ptr& o_driftInfo)
{
    int i_err = HJ_OK;
    do
    { 
        HJRteDriftInfo::Ptr driftInfo = i_link->getDriftInfo();
        if (!driftInfo)
        {
            i_err = HJErrFatal;
            break;
        }
        
        if ((driftInfo->m_srcWidth <= 0) || (driftInfo->m_srcHeight <= 0))
        {
            break;
        }    

        // i_err = tryRestart(driftInfo->m_srcWidth, driftInfo->m_srcHeight);
        // if (i_err < 0)
        // {
        //     break;
        // }
        
        HJRteComLinkInfo::Ptr linkInfo = i_link->getLinkInfo();
        HJSizei sizei = getTargetWH();
//         HJVec4i veci = linkInfo->convert(sizei.w, sizei.h);
//         int viewOffx = veci.x;
//         int viewOffy = veci.y;
//         int viewTargetW = veci.z;
//         int viewTargetH = veci.w;
// #if defined (HarmonyOS)    
//         glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);  //bindEx have been called viewport and clear color
// #endif
        std::shared_ptr<HJOGBaseShader> shader = nullptr;
        if (i_link->getShader())   //1. client special shader, for example, copy 2D, copy OES
        {
            shader = i_link->getShader();
        }
        else if (getShader())      //2. com simple shader, for example gray com
        {
            shader = getShader();
        }
        // else                    //3. com more pipeline shader, is not simple, for example blur;
        // {
            
        // }

        if (shader)
        {
            i_err = shader->draw(driftInfo->m_textureId, mapWindowRenderMode(driftInfo->m_windowRenderMode), driftInfo->m_srcWidth, driftInfo->m_srcHeight, sizei.w, sizei.h, driftInfo->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
            if (i_err < 0)
            {
                break;
            }  
        }
        else                        //3. com more pipeline shader, is not simple, for example blur;
        {
            i_err = drawEx(driftInfo->m_textureId, mapWindowRenderMode(driftInfo->m_windowRenderMode), driftInfo->m_srcWidth, driftInfo->m_srcHeight, sizei.w, sizei.h, driftInfo->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
            if (i_err < 0)
            {
                break;
            }
        }
#if defined(HarmonyOS)        
        o_driftInfo = HJRteDriftInfo::Create<HJRteDriftInfo>(m_endFbo->getTextureId(), HJRteTextureType_2D, HJRteWindowRenderMode_CLIP, sizei.w, sizei.h);
#endif
    }while (false);
    return i_err;
}
///////////////////////////////////////////////
HJRteComDrawGrayFBO::HJRteComDrawGrayFBO()
{
    HJ_SetInsName(HJRteComDrawGrayFBO);
    setPriority(HJRteComPriority_VideoGray);
}
    
int HJRteComDrawGrayFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }
#if defined(HarmonyOS)
        m_endShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Gray);
        if (nullptr == m_endShader)
        {
            i_err = HJErrFatal;
            break;
        }   
#endif
    } while (false);
    return i_err;    
}
void HJRteComDrawGrayFBO::done() 
{
    HJRteComDrawFBO::done();    
}


///////////////////////////////////////////////////
///////////////////////////////////////////////
HJRteComDrawBlurFBO::HJRteComDrawBlurFBO()
{
    HJ_SetInsName(HJRteComDrawBlurFBO);
    setPriority(HJRteComPriority_VideoBlur);
}
    
int HJRteComDrawBlurFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }   
    } while (false);
    return i_err;    
}
void HJRteComDrawBlurFBO::done() 
{
    HJRteComDrawFBO::done();    
}
int HJRteComDrawBlurFBO::adjustResolution(int i_width, int i_height)
{
    int i_err = HJ_OK;
    do 
    {
        if (!isAdjustResolution())
        {
            break;
        }
        i_err = HJRteComDrawFBO::adjustResolution(i_width, i_height);
        if (i_err < 0)
        {
            break;
        }   
        i_err = HJRteComDrawFBO::tryRestartFbo(m_pass1Fbo, i_width, i_height);
        if (i_err < 0)
        {
            break;
        }   

    } while (false);
    return i_err;    
}
// int HJRteComDrawBlurFBO::priTryRestartFbo(int i_width, int i_height, std::shared_ptr<HJOGFBOCtrl>& o_fbo)
// {
//     int i_err = HJ_OK;
//     do 
//     {
// #if defined(HarmonyOS)        
//         bool bResetFbo = false;
//         if (!o_fbo)
//         {
//             bResetFbo = true;
//         }   
//         else 
//         {
//             if ((i_width != o_fbo->getWidth()) || (i_height != o_fbo->getHeight()))
//             {
//                 bResetFbo = true;
//             }
//         }   

//         if (bResetFbo)
//         {
//             o_fbo = HJOGFBOCtrl::Create();
//             i_err = o_fbo->init(i_width, i_height);
//         }
// #endif
//     } while (false);
//     return i_err;
// }
int HJRteComDrawBlurFBO::drawEx(GLuint textureId, const std::string& i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat, bool i_bYFlip, bool i_bXFlip) 
{
    int i_err = HJ_OK;
    do 
    {
#if defined(HarmonyOS)         
        if (!m_endFbo)
        {
            i_err = HJErrFatal;
            break;
        }
        if (!m_pass1Shader)
        {
            m_pass1Shader = HJOGBaseShader::createShader(HJOGBaseShaderType_BlurHori);
            if (nullptr == m_pass1Shader)
            {
                i_err = HJErrFatal;
                break;
            }
        }
        if (!m_pass2Shader)
        {
            m_pass2Shader = HJOGBaseShader::createShader(HJOGBaseShaderType_BlurVert);
            if (nullptr == m_pass2Shader)
            {
                i_err = HJErrFatal;
                break;
            }
        }
        // i_err = priTryRestartFbo(srcw, srch, m_pass1Fbo);
        // if (i_err < 0)
        // {
        //     break;
        // }
       
        m_pass1Fbo->attach();
        {
            HJOGBaseBlurShader::Ptr shader = HJ_CvtDynamic(HJOGBaseBlurShader, m_pass1Shader);
            shader->setWidth(srcw);
            shader->setHeight(srch);
        }

        m_pass1Shader->draw(textureId, i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        m_pass1Fbo->detach();
        
        //m_endFbo->attach();  //outer already attach 
        {
            HJOGBaseBlurShader::Ptr shader = HJ_CvtDynamic(HJOGBaseBlurShader, m_pass2Shader);
            shader->setWidth(srcw);
            shader->setHeight(srch);
        }
        m_pass2Shader->draw(m_pass1Fbo->getTextureId(), i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        //m_endFbo->detach();  //outer detach 
#endif
    } while (false);
    return i_err;
}
NS_HJ_END