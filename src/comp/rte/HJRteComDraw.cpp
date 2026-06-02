#include "HJRteComDraw.h"
#include "HJFLog.h"

#include "HJRteUtils.h"
#include "HJPBOReadWrapper.h"
#include "HJOGBaseShader.h"
#include "HJOGEGLSurface.h"
#include "HJOGShaderCommon.h"
#include "HJOGFBOCtrl.h"
#include "HJFBOCtrlPool.h"
#include "HJRteGraphBaseEGL.h"


#if defined(HarmonyOS)
    #include <native_window/graphic_error_code.h>
    #include <native_image/native_image.h>
    #include <native_window/external_window.h>
    #include <native_buffer/native_buffer.h>
#endif

NS_HJ_BEGIN

HJRteComDraw::HJRteComDraw()
{
    
}
HJRteComDraw::~HJRteComDraw()
{
    
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

// int HJRteComDraw::render(HJRteDriftInfo::Ptr& o_param)
// {
//     int i_err = HJ_OK;
//     do 
//     {
// #if defined(HarmonyOS)
//         i_err = foreachPreLink([this, &o_param](const std::shared_ptr<HJRteComLink> &i_link)
//         {
//             int ret = HJ_OK;
//             do
//             { 
//                 std::string linkName = i_link->getInsName();
//                 HJFLogi("{} render linkName:{}", getInsName(), linkName);
//                 const std::shared_ptr<HJRteComLinkInfo>& linkInfo = i_link->getLinkInfo();
//                 if (!linkInfo)
//                 {
//                     ret = HJErrFatal;
//                     break;
//                 }  
                
//                 HJRteDriftInfo::Ptr driftInfo = i_link->getDriftInfo();
//                 if (!driftInfo)
//                 {
//                     ret = HJErrFatal;
//                     break;
//                 }    
                
//                 if (m_customDraw)
//                 {
//                     ret = m_customDraw(driftInfo, o_param);
//                     if (ret < 0)
//                     {
//                         break;
//                     }
//                 } 
//                 else  
//                 {
//                     HJRteComDrawFBO::Ptr fboPtr = HJ_CvtDynamic(HJRteComDrawFBO, HJBaseSharedObject::getSharedFrom(this));
//                     if (fboPtr)
//                     {
//                         fboPtr->check(driftInfo->m_srcWidth, driftInfo->m_srcHeight);
//                     }    
                    
//                     ret = attach();
//                     if (ret < 0)
//                     {
//                         break;
//                     }
//                     HJSizei sizei = getTargetWH();
//                     HJVec4i veci = linkInfo->convert(sizei.w, sizei.h);
//                     int viewOffx = veci.x;
//                     int viewOffy = veci.y;
//                     int viewTargetW = veci.z;
//                     int viewTargetH = veci.w;
//                     glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);
                       
//                     HJOGBaseShader::Ptr shader = nullptr;
//                     if (i_link->getShader())
//                     {
//                         shader = i_link->getShader();
//                         HJFLogi("{} find shader, link have shader", getInsName());
//                     }    
//                     else if (fboPtr && fboPtr->getShader())
//                     {
//                         shader = fboPtr->getShader();
//                         HJFLogi("{} find shader, fbo have shader", getInsName());
//                     } 
//                     else 
//                     {
//                         ret = HJErrFatal;
//                         HJFLoge("{} not find shader, error", getInsName());
//                         break;
//                     }
//                     ret = shader->draw(driftInfo->m_textureId, mapWindowRenderMode(linkInfo->m_renderMode), driftInfo->m_srcWidth, driftInfo->m_srcHeight, viewTargetW, viewTargetH, driftInfo->m_textureMat);
//                     if (ret < 0)
//                     {
//                         break;
//                     }   
                    
//                     ret = detach();
//                     if (ret < 0)
//                     {
//                         break;    
//                     }
                    
//                     if (fboPtr && fboPtr->isFilter())
//                     {
//                         o_param = HJRteDriftInfo::Create<HJRteDriftInfo>(fboPtr->getTextureId(), HJRteTextureType_2D, sizei.w, sizei.h);
//                     }
//                 }
// //               HJFLogi("{} com:<{} - {}> info:{} render sub #########", getInsName(), i_link->getSrcComName(), i_link->getDstComName(), i_link->getInfo());

//             } while(false);
//             return ret;
//         });
//         if (i_err < 0)
//         {
//             break;
//         }
// #endif
//     } while (false);
//     return i_err;
// }
    
// int HJRteComDraw::attach()
// {
//     return HJ_OK;
// }
// int HJRteComDraw::detach()
// {
//     return HJ_OK;
// }
HJSizei HJRteComDraw::getTargetWH()
{
    return HJSizei{0,0};
}
////////////////////////////////////////////////

int HJRteComDrawEGLImgReceiver::adjustResolution(int i_width, int i_height)
{
    int i_err = HJ_OK;
    do
    {

        HJOGEGLSurface::Ptr surface = m_eglSurfaceWtr.lock();
        if (surface)
        {
#if defined(HarmonyOS)
            OH_NativeWindow_NativeWindowHandleOpt((OHNativeWindow*)surface->getWindow(), SET_BUFFER_GEOMETRY, static_cast<int>(i_width), static_cast<int>(i_height));
#endif
            surface->setTargetWidth(i_width);
            surface->setTargetHeight(i_height);        
        }
    } while (false);
    return i_err;    
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


int HJRteComDrawEGL::bind()
{
    int i_err = HJ_OK;
    do 
    {
        HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
        if (eglSurface)
        {
            HJOGEGLSurface::HJOGEGLSurfaceFunc func = eglSurface->procMakeCurrentCb();
            if (func)
            {
                i_err = func();
				if (i_err < 0)
				{
					HJFLoge("{} makeCurrent error", getInsName());
					break;
				}
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT);
            }
        }        
    } while (false);
    return i_err;   
}

int HJRteComDrawEGL::unbind(bool i_bDraw)
{
    int i_err = HJ_OK;
    do 
    {
		HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
		if (eglSurface)
		{
            HJOGEGLSurface::HJOGEGLSurfaceFunc func = eglSurface->procSwapCb();
            if (func)
            {
                i_err = func();
				if (i_err < 0)
				{
					HJFLoge("{} swap error", getInsName());
					break;
				}
            }		
		}
    } while (false);
    return i_err;
}
int HJRteComDrawEGL::render(const std::shared_ptr<HJRteComLink>&i_link, const HJRteDriftInfo::Ptr& i_drift)
{
    int i_err = HJ_OK;
    do
    {
        HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
        if (!eglSurface)
        {
            break;
        }        
        if (!i_drift)
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

        HJFPERLog5i("{} tryResolution render textureId:{} renderMode:{} srcWidth:{} srcHeight:{} viewTargetW:{} viewTargetH:{}", getInsName(), i_drift->m_textureId, linkInfo->m_renderMode, i_drift->m_srcWidth, i_drift->m_srcHeight, viewTargetW, viewTargetH);

        glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);
        i_err = shader->draw(i_drift->m_textureId, linkInfo->m_renderMode, i_drift->m_srcWidth, i_drift->m_srcHeight, viewTargetW, viewTargetH, i_drift->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
        if (i_err < 0)
        {
            break;
        }   
        
    } while (false);
    return i_err;
}
// int HJRteComDrawEGL::attach()
// {
//     int i_err = HJ_OK;
//     do 
//     {
// #if defined (HarmonyOS)
//         HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
//         if (eglSurface)
//         {
//             i_err = eglSurface->procMakeCurrentCb()();
//             if (i_err < 0)
//             {
//                 HJFLoge("{} makeCurrent error", getInsName());
//                 break;
//             }    
//         }
        
//         if (m_curRenderIdx == 0)
//         {

//             glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
// 		    glClear(GL_COLOR_BUFFER_BIT);

//         }
//         m_curRenderIdx++;
// #endif        
//     } while (false);
//     return i_err;
// }
// void HJRteComDrawEGL::reset()
// {
//     m_curRenderIdx = 0;
// }
// int HJRteComDrawEGL::detach()
// {
//     int i_err = HJ_OK;
//     do {
//         int nPreSize = m_preQueue.size();
//         if (m_curRenderIdx == nPreSize)
//         {
// #if defined (HarmonyOS)
//             HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
//             if (eglSurface)
//             {
//                 i_err = eglSurface->procSwapCb()();
//                 if (i_err < 0)
//                 {
//                     HJFLoge("{} swap error", getInsName());
//                     break;
//                 } 
//             }
// #endif
//         }    
//     } while (false);
//     return i_err;
// }

HJSizei HJRteComDrawEGL::getTargetWH() 
{
    HJOGEGLSurface::Ptr eglSurface = m_eglSurfaceWtr.lock();
    if (eglSurface)
    {
        return HJSizei{eglSurface->getTargetWidth(), eglSurface->getTargetHeight()};
    }    
    return HJSizei{0,0};
}


void HJRteComDrawEGL::setSurface(const std::shared_ptr<HJOGEGLSurface>& i_eglSurface)
{
    m_eglSurfaceWtr = i_eglSurface;
}
std::shared_ptr<HJOGEGLSurface> HJRteComDrawEGL::getSurface()
{
    return m_eglSurfaceWtr.lock();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HJRteComDrawEGLUI_0::HJRteComDrawEGLUI_0()
{
    HJ_SetInsName(HJRteComDrawEGLUI_0);
    setPriority(HJRteComPriority_Target);
}
HJRteComDrawEGLUI_1::HJRteComDrawEGLUI_1()
{
    HJ_SetInsName(HJRteComDrawEGLUI_1);
    setPriority(HJRteComPriority_Target);
}

HJRteComDrawEGLUI_2::HJRteComDrawEGLUI_2()
{
    HJ_SetInsName(HJRteComDrawEGLUI_2);
    setPriority(HJRteComPriority_Target);
}

HJRteComDrawEGLUI_3::HJRteComDrawEGLUI_3()
{
    HJ_SetInsName(HJRteComDrawEGLUI_3);
    setPriority(HJRteComPriority_Target);
}


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
        if (i_param)
        {
            HJ_CatchMapPlainGetVal(i_param, bool, "forceXMirror", m_forceXMirror);
            HJ_CatchMapPlainGetVal(i_param, bool, "forceYMirror", m_forceYMirror);
        }
    } while (false);
    return i_err;    
}
void HJRteComDrawFBO::done() 
{
    if (m_dynamicFbo)
    {
        HJFLogi("{} resource release", getInsName());
        HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(m_dynamicFbo);
        m_dynamicFbo = nullptr;
    }
    if (m_endShader)
    {
        m_endShader.reset();
        m_endShader = nullptr;
    }    
    HJRteComDraw::done();
}

HJSizei HJRteComDrawFBO::getTargetWH() 
{
    if (m_dynamicFbo)
    {
        return HJSizei{ m_dynamicFbo->getWidth(), m_dynamicFbo->getHeight()};
    }
    if (m_adjustWidth > 0 && m_adjustHeight > 0)
    {
        return HJSizei{ m_adjustWidth, m_adjustHeight };
    }
    return HJSizei{0, 0};
}

int HJRteComDrawFBO::getWidth()
{
    if (m_dynamicFbo)
    {
        return m_dynamicFbo->getWidth();
    }
    return 0;
}
int HJRteComDrawFBO::getHeight()
{
    if (m_dynamicFbo)
    {
        return m_dynamicFbo->getHeight();
    }
    return 0;
}
GLuint HJRteComDrawFBO::getTextureId()
{
    if (m_dynamicFbo)
    {
        return m_dynamicFbo->getTextureId();
    }
    return 0;
}
//
//int HJRteComDrawFBO::check(int i_width, int i_height, bool i_bTransparency)
//{
//    int i_err = HJ_OK;
//#if defined(HarmonyOS)
//    if ((i_width > 0) && (i_height > 0))
//    {
//        bool bResetFbo = false;
//        if (!m_endFbo)
//        {
//            bResetFbo = true;
//        }   
//        else 
//        {
//            if ((i_width != m_endFbo->getWidth()) || (i_height != m_endFbo->getHeight()))
//            {
//                bResetFbo = true;
//            }
//        }   
//
//        if (bResetFbo)
//        {
//            m_endFbo = HJOGFBOCtrl::Create();
//            HJFLogi("{} fbo change width:{} height:{}", getInsName(), i_width, i_height);
//            i_err = m_endFbo->init(i_width, i_height, i_bTransparency);
//        }
//    }   
//#endif
//    return i_err;
//}

int HJRteComDrawFBO::bind()
{
    int i_err = HJ_OK;
    do 
    {
        m_isBinded = true;
        if (!m_dynamicFbo)
        {
            //HJFLogi("{} acquire FBO this com width:{} height:{}", getInsName(), m_adjustWidth, m_adjustHeight);
            m_dynamicFbo = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire(getInsName(), m_adjustWidth, m_adjustHeight, m_adjustTransparency);
            if (!m_dynamicFbo)
            {
                i_err = HJ_WOULD_BLOCK;
                HJFLoge("{} fbo bind tmpFBO == nullptr why?", getInsName());
                break;
            }
        }

        //for example, PBO use static FBO, so need reset FBO when change size;
        if ((m_adjustWidth != m_dynamicFbo->getWidth()) || (m_adjustHeight != m_dynamicFbo->getHeight()))
        {
            HJFLogi("{} fbo change width:{} height:{}", getInsName(), m_adjustWidth, m_adjustHeight);
            HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(m_dynamicFbo);
            m_dynamicFbo = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire(getInsName(), m_adjustWidth, m_adjustHeight, m_adjustTransparency);
            if (!m_dynamicFbo)
            {
                i_err = HJ_WOULD_BLOCK;
                HJFLoge("{} fbo bind tmpFBO == nullptr why?", getInsName());
                break;
            }
        }

        m_dynamicFbo->attach();
    } while (false);
    return i_err; 
}
int HJRteComDrawFBO::unbind(bool i_bDraw)
{
    int i_err = HJ_OK;
    do 
    {
        m_isBinded = false;
        if (!m_dynamicFbo)
        {
            i_err = HJ_WOULD_BLOCK;
            HJFLoge("{} fbo unbind tmpFBO == nullptr why?", getInsName());
            break;
        }
        m_dynamicFbo->detach();
    } while (false);
    return i_err;
}

std::shared_ptr<HJOGFBOCtrl> HJRteComDrawFBO::takeFbo()
{
    std::shared_ptr<HJOGFBOCtrl> ret = nullptr;
    if (m_dynamicFbo)
    {
        ret = std::move(m_dynamicFbo);
        m_dynamicFbo = nullptr;
    }
    return ret;
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
        m_adjustWidth = i_width;
        m_adjustHeight = i_height;
        m_adjustTransparency = true;
        //i_err = HJRteComDrawFBO::tryRestartFbo(m_endFbo, i_width, i_height);
    } while (false);
    return i_err;
}
int HJRteComDrawFBO::tryRestartFbo(std::shared_ptr<HJOGFBOCtrl>& o_fbo, int i_width, int i_height, bool i_bTranspareny)
{
    int i_err = HJ_OK;
    do 
    {        
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
    } while (false);
    return i_err; 
}

int HJRteComDrawFBO::render(const std::shared_ptr<HJRteComLink>&i_link, const HJRteDriftInfo::Ptr& i_drift)
{
    int i_err = HJ_OK;
    do
    { 
        if (!i_drift)
        {
            i_err = HJErrFatal;
            break;
        }
        
        if ((i_drift->m_srcWidth <= 0) || (i_drift->m_srcHeight <= 0))
        {
            break;
        }    

        //if (!isEnable())
        //{
        //    //HJRteCom::setDriftInfo(driftInfo, false);
        //    break;
        //}
                
        HJRteComLinkInfo::Ptr linkInfo = i_link->getLinkInfo();
        HJSizei sizei = getTargetWH();
		HJVec4i veci = linkInfo->convert(sizei.w, sizei.h);
		int viewTargetW = sizei.w;
		int viewTargetH = sizei.h;
        //if use fbo local canvas, can open this, must be target, block render;
#if 0
		int viewOffx = veci.x;
		int viewOffy = veci.y;
		int viewTargetW = veci.z;
		int viewTargetH = veci.w;
		glViewport(viewOffx, viewOffy, viewTargetW, viewTargetH);  //bind have been called viewport and clear color
#endif
        std::shared_ptr<HJOGBaseShader> shader = nullptr;
        if (i_link->getShader())   //1. client special shader, for example, copy 2D, copy OES
        {
            shader = i_link->getShader();
        }
        else if (getShader())      //2. com simple shader, for example gray com
        {
            shader = getShader();
        }
        // else                    //3. com more pipeline shader, is not simple, for example blur; or customer callback filter 
        // {       
        // }

        if (shader)
        {
            i_err = shader->draw(i_drift->m_textureId, linkInfo->m_renderMode, i_drift->m_srcWidth, i_drift->m_srcHeight, viewTargetW, viewTargetH, i_drift->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
            if (i_err < 0)
            {
                break;
            }  
        }
        else                        //3. com more pipeline shader, is not simple, for example blur;
        {
            i_err = draw(i_drift->m_textureId, linkInfo->m_renderMode, i_drift->m_srcWidth, i_drift->m_srcHeight, viewTargetW, viewTargetH, i_drift->m_textureMat, linkInfo->m_yMirror, linkInfo->m_xMirror);
            if (i_err < 0)
            {
                break;
            }
        }        
        //HJRteDriftInfo::Ptr drift = HJRteDriftInfo::Create<HJRteDriftInfo>(getTextureId(), HJRteTextureType_2D, sizei.w, sizei.h);
        //drift->setSrcComName(getInsName());
        //HJRteCom::setDriftInfo(drift);
    }while (false);

    return i_err;
}

///////////////////////////////////////////////
HJRteComDrawCopy2DFBO::HJRteComDrawCopy2DFBO()
{   
    HJ_SetInsName(HJRteComDrawCopy2DFBO);
    setPriority(HJRteComPriority_VideoSource);    
}
int HJRteComDrawCopy2DFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        m_endShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D, m_forceXMirror, m_forceYMirror);
        if (nullptr == m_endShader)
        {
            i_err = HJErrFatal;
            break;
        }   
    } while (false);
    return i_err;       
}

/////////////////////////////////////////////////

HJRteComDrawCopyOESFBO::HJRteComDrawCopyOESFBO()
{   
    HJ_SetInsName(HJRteComDrawCopyOESFBO);
    setPriority(HJRteComPriority_VideoSource);    
}
int HJRteComDrawCopyOESFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        m_endShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_OES, m_forceXMirror, m_forceYMirror);
        if (nullptr == m_endShader)
        {
            i_err = HJErrFatal;
            break;
        }   
    } while (false);
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
        m_endShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Gray, m_forceXMirror, m_forceYMirror);
        if (nullptr == m_endShader)
        {
            i_err = HJErrFatal;
            break;
        }   
    } while (false);
    return i_err;    
}
void HJRteComDrawGrayFBO::done() 
{
    HJRteComDrawFBO::done();    
}

////////////////////////////////////////////
HJRteComDrawXMirrorFBO::HJRteComDrawXMirrorFBO()
{
    HJ_SetInsName(HJRteComDrawXMirrorFBO);
    setPriority(HJRteComPriority_Mirror);    
}
int HJRteComDrawXMirrorFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        m_endShader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D, m_forceXMirror, m_forceYMirror);
        if (nullptr == m_endShader)
        {
            i_err = HJErrFatal;
            break;
        }   

        HJOGCopyShaderStrip::Ptr shader = HJ_CvtDynamic(HJOGCopyShaderStrip, m_endShader);
        if (shader)
        {
            shader->setForceXMirror(true);
        }
    } while (false);
    return i_err;        
}

///////////////////////////////////////////////

HJRteBlurHolder::~HJRteBlurHolder()
{
    if (m_holdFbo)
    {
        HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(m_holdFbo);
        m_holdFbo = nullptr;
    }
}
std::shared_ptr<HJOGFBOCtrl> HJRteBlurHolder::getFBOCtr()
{
    return m_holdFbo;
}
int HJRteComDrawBlurKernal::draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat, bool i_bYFlip, bool i_bXFlip)
{
    int i_err = HJ_OK;
    do 
    {        
        //not use m_endFbo, only simple com use, for example, grapy use m_endFbo

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

        HJOGFBOCtrl::Ptr pass1Fbo = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire(getInsName(), srcw, srch);
        if (!pass1Fbo)
        {
            return HJ_WOULD_BLOCK;
        }
        HJOGFBOCtrl::Ptr pass2Fbo = HJRteGraphBaseEGL::getFBOCtrlPool()->acquire(getInsName(), srcw, srch);
		if (!pass2Fbo)
		{
			return HJ_WOULD_BLOCK;
		}

        pass1Fbo->attach();
        {
            HJOGBaseBlurShader::Ptr shader = HJ_CvtDynamic(HJOGBaseBlurShader, m_pass1Shader);
            shader->setWidth(srcw);
            shader->setHeight(srch);
        }

        m_pass1Shader->draw(textureId, i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        pass1Fbo->detach();
        
        pass2Fbo->attach();  
        {
            HJOGBaseBlurShader::Ptr shader = HJ_CvtDynamic(HJOGBaseBlurShader, m_pass2Shader);
            shader->setWidth(srcw);
            shader->setHeight(srch);
        }
        m_pass2Shader->draw(pass1Fbo->getTextureId(), i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        pass2Fbo->detach();  //outer detach 

        HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(pass1Fbo);

        m_holder = HJRteBlurHolder::Create<HJRteBlurHolder>(pass2Fbo);
    } while (false);
    return i_err;
}
std::shared_ptr<HJRteBlurHolder> HJRteComDrawBlurKernal::getHolder()
{
    if (m_holder)
    {
		return std::move(m_holder);
	}
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////
HJRteComDrawBlurCascadeFBO::HJRteComDrawBlurCascadeFBO()
{
    HJ_SetInsName(HJRteComDrawBlurCascadeFBO);
    setPriority(HJRteComPriority_VideoBlur);
}
    
int HJRteComDrawBlurCascadeFBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do 
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }   
        for (int i = 0; i < m_cascadeNum; i++)
        {
            HJRteComDrawBlurKernal::Ptr blurFbo = HJRteComDrawBlurKernal::Create();
            blurFbo->setInsName(HJFMT("HJRteComDrawBlurKernal_{}", i));
            m_blurVector.push_back(blurFbo);
        }
        m_shader = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D, m_forceXMirror, m_forceYMirror);
        if (nullptr == m_shader)
        {
            i_err = HJErrFatal;
            break;
        }

    } while (false);
    return i_err;
}
void HJRteComDrawBlurCascadeFBO::done() 
{
    HJRteComDrawFBO::done();    
}
int HJRteComDrawBlurCascadeFBO::draw(GLuint i_textureId, int i_fitMode, int i_srcw, int i_srch, int i_dstw, int i_dsth, float* i_texMat, bool i_bYFlip, bool i_bXFlip) 
{
    int i_err = HJ_OK;
    do
    {
        GLuint textureId = i_textureId;
        int fitMode = i_fitMode;
        int srcw = i_srcw;
        int srch = i_srch;
        int dstw = i_dstw;
        int dsth = i_dsth;
        float* texMat = i_texMat;
        bool bYFlip = i_bYFlip;
        bool bXFlip = i_bXFlip;
        HJRteBlurHolder::Ptr holder = nullptr;
        for (auto& blurFbo : m_blurVector)
        {
            i_err = blurFbo->draw(textureId, fitMode, srcw, srch, dstw, dsth, texMat, bYFlip, bXFlip);
            if (i_err < 0)
            {
                break;
            }
            holder = blurFbo->getHolder(); 
            textureId = holder->getFBOCtr()->getTextureId();
        }

        i_err = m_shader->draw(holder->getFBOCtr()->getTextureId(), i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        //i_err = m_shader->draw(textureId, i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

//HJSizei HJRteComDrawBlurCascadeFBO::getTargetWH()  
//{
//    if (!m_blurVector.empty())
//    {
//        return m_blurVector.back()->getTargetWH();
//    }
//    return HJSizei{0, 0};
//}
//GLuint HJRteComDrawBlurCascadeFBO::getTextureId() 
//{
//    if (!m_blurVector.empty())
//    {
//        return m_blurVector.back()->getTextureId();
//    }
//    return 0;
//}
//
//int HJRteComDrawBlurCascadeFBO::getWidth() 
//{
//    if (!m_blurVector.empty())
//    {
//        return m_blurVector.back()->getWidth();
//    }
//    return 0;
//}
//int HJRteComDrawBlurCascadeFBO::getHeight() 
//{
//    if (!m_blurVector.empty())
//    {
//        return m_blurVector.back()->getHeight();
//    }
//    return 0;
//}

///////////////////////////////////////////////////////////////////////////////////////
HJRteComDrawPBOFBO::HJRteComDrawPBOFBO()
{
    HJ_SetInsName(HJRteComDrawPBOFBO);
    setPriority(HJRteComPriority_Target);
}
HJRteComDrawPBOFBO::~HJRteComDrawPBOFBO()
{

}
int HJRteComDrawPBOFBO::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        if (!m_readCb)
        {
            HJ_CatchMapGetVal(i_param, HJMediaDataReaderCb, m_readCb);
        }     
    } while (false);
    return i_err;
}
// int HJRteComDrawPBOFBO::adjustResolution(int i_width, int i_height) 
// {
//     int i_err = HJ_OK;
//     do
//     {
//         i_err = HJRteComDrawFBO::adjustResolution(i_width, i_height);
//         if (i_err < 0)
//         {
//             break;
//         }
        
//         if ((m_catchWidth != m_adjustWidth) || (m_catchHeight != m_adjustHeight))
//         {
//             m_catchWidth = m_adjustWidth;
//             m_catchHeight = m_adjustHeight;
//             if (m_dynamicFbo)
//             {
//                 HJRteGraphBaseEGL::getFBOCtrlPool()->recovery(m_dynamicFbo);
//                 m_dynamicFbo = nullptr;
//             }
//         }

//     } while (false);
//     return i_err;
// }
int HJRteComDrawPBOFBO::unbind(bool i_bDraw)
{
    int i_err = HJ_OK;
    do
    {
        if (i_bDraw)
        {
            i_err = priReadPBO();
            if (i_err < 0)
            {
                HJRteComDrawFBO::unbind(i_bDraw);
                break;
            }
        }
        HJRteComDrawFBO::unbind(i_bDraw);
    } while (false);
    return i_err;
}
int HJRteComDrawPBOFBO::priReadPBO()
{
    int i_err = HJ_OK;
    do
    {
        if (!m_pboReader)
        {
            m_pboReader = HJPBOReadWrapper::Create();
            m_pboReader->setReadCb(m_readCb);
        }
        i_err = m_pboReader->process(m_adjustWidth, m_adjustHeight);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJRteComDrawPBOFBO::setReadCb(HJMediaDataReaderCb i_cb)
{
    m_readCb = i_cb;
    if (m_pboReader)
    {
        m_pboReader->setReadCb(m_readCb);
    }
}
//////////////////////////////////////////////////////////
HJRteComDrawPBOFBODetect::HJRteComDrawPBOFBODetect()
{
    HJ_SetInsName(HJRteComDrawPBOFBODetect);
    setPriority(HJRteComPriority_Target);
}
//////////////////////////////////////////////////////////
HJRteComDrawPBOFBOTarget_0::HJRteComDrawPBOFBOTarget_0()
{
    HJ_SetInsName(HJRteComDrawPBOFBOTarget_0);
    setPriority(HJRteComPriority_Target);
}
HJRteComDrawPBOFBOTarget_1::HJRteComDrawPBOFBOTarget_1()
{
    HJ_SetInsName(HJRteComDrawPBOFBOTarget_1);
    setPriority(HJRteComPriority_Target);
}
HJRteComDrawPBOFBOTarget_2::HJRteComDrawPBOFBOTarget_2()
{
    HJ_SetInsName(HJRteComDrawPBOFBOTarget_2);
    setPriority(HJRteComPriority_Target);
}
HJRteComDrawPBOFBOTarget_3::HJRteComDrawPBOFBOTarget_3()
{
    HJ_SetInsName(HJRteComDrawPBOFBOTarget_3);
    setPriority(HJRteComPriority_Target);
}

//////////////////////////////////////////////////////////
HJRteComCustomSourceFilter::HJRteComCustomSourceFilter()
{
    HJ_SetInsName(HJRteComCustomSourceFilter);
    setPriority(HJRteComPriority_VideoSourceCustomFilter);
}
HJRteComCustomSourceFilter::~HJRteComCustomSourceFilter()
{
    if (m_releaseFunc)
    {
        m_releaseFunc();
    }
}

int HJRteComCustomSourceFilter::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    { 
        HJ_CatchMapGetVal(i_param, HJCustomSourceFilterInit, m_initFunc);
        HJ_CatchMapGetVal(i_param, HJCustomSourceFilterDraw, m_drawFunc);
        HJ_CatchMapGetVal(i_param, HJCustomSourceFilterRelease, m_releaseFunc);

        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }   

        if (m_initFunc)
        {
            i_err = m_initFunc();
            if (i_err < 0)
            {
                break;
            }
        }

    } while (false);
    return i_err;
}

int HJRteComCustomSourceFilter::adjustResolution(int i_width, int i_height)
{
    m_customWidth = i_width;
    m_customHeight = i_height;
    return HJ_OK;
}

HJSizei HJRteComCustomSourceFilter::getTargetWH()
{
    return HJSizei{m_customWidth, m_customHeight};
}
int HJRteComCustomSourceFilter::bind()
{
    return HJ_OK;
}
int HJRteComCustomSourceFilter::unbind(bool i_bDraw)
{
    return HJ_OK;
}
GLuint HJRteComCustomSourceFilter::getTextureId()
{
    return m_outTextureId;
}
int HJRteComCustomSourceFilter::draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat, bool i_bYFlip, bool i_bXFlip)
{
    int i_err = HJ_OK;
    do
    {
        if (m_drawFunc)
        {
            i_err = m_drawFunc(textureId, m_outTextureId, srcw, srch);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}

int HJRteComCustomSourceFilter::getWidth()
{
    return m_customWidth;
}
int HJRteComCustomSourceFilter::getHeight()
{
    return m_customHeight;
}

NS_HJ_END
