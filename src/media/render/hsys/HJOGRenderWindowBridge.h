#pragma once

#include "HJPrerequisites.h"
#include "HJOGCopyShaderStrip.h"
#include "HJTransferInfo.h"
#include "HJOGCopyShaderStrip.h"
#include <deque>
#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <native_window/graphic_error_code.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>
#endif

NS_HJ_BEGIN

typedef enum HJOGRenderWindowBridgeState
{
    HJOGRenderWindowBridgeState_Idle = 0,
    HJOGRenderWindowBridgeState_Avaiable,
    HJOGRenderWindowBridgeState_Ready,
} HJOGRenderWindowBridgeState;

class HJOGRenderWindowBridge
{
public:
    HJ_DEFINE_CREATE(HJOGRenderWindowBridge);
    HJOGRenderWindowBridge();
    virtual ~HJOGRenderWindowBridge();

    void setInsName(const std::string &i_insName)
    {
        m_insName = i_insName;
    }
    int init();
    void done();
    int update();  
    
    int width();
    int height();
    int draw(HJTransferRenderModeInfo::Ptr i_renderModeInfo, int i_targetWidth, int i_targetHeight);
    int getSurfaceId(uint64_t &o_surfaceId) const;
    int produceFromPixel(HJTransferRenderModeInfo::Ptr i_renderModeInfo, uint8_t* i_pData[3], int i_size[3], int i_width, int i_height);
    
    bool IsStateAvaiable();
    bool IsStateReady();
    float *getTexMatrix()
    {
        return m_matrix;
    }
#if defined(HarmonyOS)
    OHNativeWindow* getNativeWindow() const
    {
        return m_nativeWindow;
    }
    GLuint getTextureId() const 
    {
        return m_texture;
    }
#endif

private:
    static void priOnFrameAvailable(void *context);
    void priSetAvailable();
    
    std::string m_insName = "";
    float m_matrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};
#if defined(HarmonyOS)
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
    bool m_bTextureReady = false;
	GLuint m_texture = 0;
    OH_NativeImage* m_nativeImage = nullptr;
    OHNativeWindow *m_nativeWindow = nullptr;
#endif
    int priUpdate();
    int priDraw(HJTransferRenderModeInfo::Ptr i_renderModeInfo, int i_targetWidth, int i_targetHeight);
    int m_srcWidth = 0;
    int m_srcHeight = 0;
    
    int m_cacheWidth = 0;
    int m_cacheHeight = 0;
    
    std::atomic<int> m_state{(int)HJOGRenderWindowBridgeState_Idle};
    //std::atomic<bool> m_bReady{false};
    bool m_bAvaiableFlag = false;
};

NS_HJ_END