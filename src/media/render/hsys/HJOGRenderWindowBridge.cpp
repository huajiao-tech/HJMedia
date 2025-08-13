#include "HJOGRenderEnv.h"
#include "HJFLog.h"
#include "HJThreadPool.h"
#include "HJOGEGLCore.h"

#include "libyuv.h"

#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>    
#include <sys/mman.h>
#include "HJOGCopyShaderStrip.h"
#endif

#include "HJOGRenderWindowBridge.h"

NS_HJ_BEGIN

HJOGRenderWindowBridge::HJOGRenderWindowBridge()
{

}
HJOGRenderWindowBridge::~HJOGRenderWindowBridge()
{
    HJFLogi("{}, ~HJOGRenderWindowBridge enter, {}", m_insName, size_t(this));
}

void static pri_OH_OnFrameAvailable(void *context)
{
    HJOGRenderWindowBridge*the = (HJOGRenderWindowBridge*)context;
    the->m_bRead = true;
}
int HJOGRenderWindowBridge::getSurfaceId(uint64_t &o_surfaceId) const
{
    int i_err = 0;
    do 
    {
#if defined(HarmonyOS)
        if (m_nativeImage)
        {
            i_err = OH_NativeImage_GetSurfaceId(m_nativeImage, &o_surfaceId);   
            if (i_err != 0)
            {
                HJFLoge("{} OH_NativeImage_GetSurfaceId error i_err:{}", m_insName, i_err);
                i_err  = -1;
                break;
            }
            HJFLogi("{} OH_NativeImage_GetSurfaceId id:{}", m_insName, o_surfaceId);
        }
#endif
    } while (false);
    return i_err;

}
int HJOGRenderWindowBridge::init(const std::string &i_renderModeInfo)
{
    int i_err = 0;
    do          
    {
#if defined(HarmonyOS)
        glGenTextures(1, &m_texture);
        m_bTextureReady = true;
            	
        uint32_t target = GL_TEXTURE_EXTERNAL_OES;

    	glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  
        glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
        
        m_nativeImage = OH_NativeImage_Create(m_texture, target);
        
        //have no effect, why after try
//        i_err = OH_NativeImage_SetDropBufferMode(m_nativeImage, true);
//        if (i_err < 0)
//        {
//            break;
//        }
        
        OH_OnFrameAvailableListener listener;
        listener.context = this;
        listener.onFrameAvailable = pri_OH_OnFrameAvailable;
        OH_NativeImage_SetOnFrameAvailableListener(m_nativeImage, listener);
        m_nativeWindow = OH_NativeImage_AcquireNativeWindow(m_nativeImage);
#endif
        i_err = m_renderModeInfo.init(i_renderModeInfo);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}
void HJOGRenderWindowBridge::done()
{
    HJFLogi("{}, done enter", m_insName);
#if defined(HarmonyOS)        
    if (m_bTextureReady)
    {
        glDeleteTextures(1, &m_texture);
        m_bTextureReady = false;
    }
    HJFLogi("{}, nativeImage destroy enter", m_insName);
    if (m_nativeImage)
    {
        OH_NativeImage_Destroy(&m_nativeImage);
        m_nativeImage = nullptr;
    }
    HJFLogi("{}, nativewindow destroy enter", m_insName);
    if (m_nativeWindow)
    {
        OH_NativeWindow_DestroyNativeWindow(m_nativeWindow);
        m_nativeWindow = nullptr;
    }
#endif
    HJFLogi("{}, done end", m_insName);
}
int HJOGRenderWindowBridge::priDraw(int i_targetWidth, int i_targetHeight)
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)
		if (!m_draw)
		{
			m_draw = HJOGCopyShaderStrip::Create();

            int nFlag = OGCopyShaderStripFlag_OES;
            if (m_renderModeInfo.bAlphaVideoLeftRight)
            {
                nFlag |= OGCopyShaderStripFlag_AlphaLeftRight;
            }
			i_err = m_draw->init(nFlag);
			HJFLogi("Draw init i_err:{}", i_err);
		}
		//HJFLogi("priShaderDraw draw");
        
        int srcWidth = m_srcWidth;
        int srcHeight = m_srcHeight;
        if (m_renderModeInfo.bAlphaVideoLeftRight)
        {
            srcWidth /= 2;
        }
		i_err = m_draw->draw(m_texture, m_renderModeInfo.cropMode, srcWidth, srcHeight, i_targetWidth, i_targetHeight, m_matrix, false);
#endif
    } while (false);
    return i_err;
}
int HJOGRenderWindowBridge::priUpdate()
{
    int i_err = 0;
    do
    {
#if defined(HarmonyOS)        
		i_err = OH_NativeImage_UpdateSurfaceImage(m_nativeImage);
		if (i_err != 0)
		{
			if (NATIVE_ERROR_NO_BUFFER == i_err)
			{
				i_err = 0; //JFLogw("Update surface image warning:40601000 {} m_texture:{} wdith:{} height:{}", i_err, m_texture, m_width, m_height);
			}
			else
			{
				i_err = -1;
				HJFLoge("Update surface image error:{} m_texture:{}", i_err, m_texture);
				break;
			}
		}
		else
		{
			int64_t timeStamp = OH_NativeImage_GetTimestamp(m_nativeImage);
			//HJFLogi("mmap Update surface timestamp:{}", timeStamp);
			i_err = OH_NativeImage_GetTransformMatrixV2(m_nativeImage, m_matrix);
			if (i_err != 0) 
            {
				i_err = -1;
				HJFLoge("OH_NativeImage_GetTransformMatrix error:{}", i_err);
				break;
			}
            ///////////////hw decode also set cache the width and height,else this get width and height is 0
            OH_NativeWindow_NativeWindowHandleOpt(m_nativeWindow, GET_BUFFER_GEOMETRY, &m_srcHeight, &m_srcWidth);
            if ((m_srcWidth == 0) || (m_srcHeight == 0))
            {
                HJFLoge("OH_NativeWindow_NativeWindowHandleOpt getwh w:{} h:{} error <producer, pixel, hwdecode, camera must SET_BUFFER_GEOMETRY > timestamp:{} ", m_srcWidth, m_srcHeight, timeStamp);
            }
            //HJFLogi("mmap Update surface timestamp:{} w:{} h:{}", timeStamp, m_srcWidth, m_srcHeight);
		}
#endif
    } while (false);
    return i_err;
}

int HJOGRenderWindowBridge::update()
{
    int i_err = 0;
    do
    {
		if (!m_bRead)
		{
			break;
		}
        i_err = priUpdate();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
    
}
int HJOGRenderWindowBridge::draw(int i_targetWidth, int i_targetHeight)
{
    int i_err = 0;
    do
    {
        if (!m_bRead)
        {
            break;
        }

        int x = i_targetWidth * m_renderModeInfo.viewOffx;
        int y = (1.f - m_renderModeInfo.viewHeight - m_renderModeInfo.viewOffy) * i_targetHeight;
        int w = i_targetWidth * m_renderModeInfo.viewWidth;
        int h = i_targetHeight * m_renderModeInfo.viewHeight;

#if defined(HarmonyOS)
        glViewport(x, y, w, h);
#endif
        //i_err = priDraw(i_targetWidth, i_targetHeight);
        i_err = priDraw(w, h);
        if (i_err < 0)
        {
            break;
        }   
    } while (false);
    return i_err;
}
int HJOGRenderWindowBridge::produceFromPixel(uint8_t* i_pData[3], int i_size[3], int i_width, int i_height)
{
	int i_err = 0;
	do
	{
#if defined(HarmonyOS)
		OHNativeWindow* window = getNativeWindow();
		if (m_cacheWidth != i_width || m_cacheHeight != i_height)
		{
			m_cacheWidth = i_width;
			m_cacheHeight = i_height;

			int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY, m_cacheWidth, m_cacheHeight);

			ret = OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);

			int code = SET_USAGE;
			int32_t usage = NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE | NATIVEBUFFER_USAGE_MEM_DMA;
			ret = OH_NativeWindow_NativeWindowHandleOpt(window, code, usage);
		}
		OHNativeWindowBuffer* buffer = nullptr;
		int fenceFd = 0;
		OH_NativeWindow_NativeWindowRequestBuffer(window, &buffer, &fenceFd);
		if (buffer)
		{
			BufferHandle* handle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
			void* mappedAddr = mmap(handle->virAddr, handle->size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

			if (mappedAddr != MAP_FAILED)
			{
				unsigned char* pixel = static_cast<unsigned  char*>(mappedAddr);
				if ((handle->width == m_cacheWidth) && (handle->height == m_cacheHeight))
				{
					//HJFLogi("mmap mathc w:{} h:{} <w*4:{} stride:{}> size:{} ", handle->width, handle->height, (4 * handle->width), handle->stride, handle->size);
					// if (m_changeState == CHANGE_STATE_SET_START)
					// {
					//     m_changeState = CHANGE_STATE_SET_END;
					// }    
                    if (m_renderModeInfo.color == "BT601")
                    {
	                  i_err = libyuv::I420ToABGR(
							i_pData[0], i_size[0],
							i_pData[1], i_size[1],
							i_pData[2], i_size[2],
							pixel, handle->stride,  
							m_cacheWidth, m_cacheHeight);
                    } 
                    else 
                    {
                        i_err = libyuv::H420ToABGR(
						    i_pData[0], i_size[0],
						    i_pData[1], i_size[1],
						    i_pData[2], i_size[2],
						    pixel, handle->stride,  
						    m_cacheWidth, m_cacheHeight);
                    }
                    if (i_err < 0)
                    {
                        HJFLoge("{} libyuv error", m_insName);
                        break;
                    }
				}
				else
				{
					HJFLoge("{} map is not match, origin <{} {} > cur:<{} {} >", m_insName, handle->width, handle->height, m_cacheWidth, m_cacheHeight);
				}
			}
			else
			{
				HJFLoge("mmap failed");
			}

			int result = munmap(mappedAddr, handle->size);
			if (result == -1)
			{
				HJFLoge("{} munmap failed result:{}", m_insName, result);
			}

			// if (m_changeState == CHANGE_STATE_SET_END)
			// {
			//     if (m_func)
			//     {
			//         m_func(timestamp, m_width, m_height);
			//         HJFLogi("change set success w:{} h:{}", m_width, m_height);
			//     }  
			//     m_changeState = CHANGE_STATE_IDLE;
			// }  

				//HJFLogi("draw enter5");
			Region region{ nullptr, 0 };
			OH_NativeWindow_NativeWindowFlushBuffer(window, buffer, fenceFd, region);
		}
		else
		{
			HJFLoge("OH_NativeWindow_NativeWindowRequestBuffer EMPTYR");
		}
#endif
	} while (false);
	return i_err;
}

NS_HJ_END