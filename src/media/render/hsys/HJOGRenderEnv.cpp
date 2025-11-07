#include "HJOGRenderEnv.h"
#include "HJFLog.h"
#include "HJThreadPool.h"
#include "HJOGEGLCore.h"
#include "HJTransferInfo.h"

#include "HJOGRenderWindowBridge.h"

#if defined(HarmonyOS)
    #include <EGL/egl.h>
    #include <GLES3/gl3.h>  
    #include "HJOGShaderCommon.h"
#endif

NS_HJ_BEGIN

HJOGRenderEnv::HJOGRenderEnv()
{

}
HJOGRenderEnv::~HJOGRenderEnv()
{
    HJFLogi("{}, ~HJOGRenderEnv enter", m_insName);
}
void HJOGRenderEnv::done()
{
    priCoreDone();
}
bool HJOGRenderEnv::isNeedEglSurface()
{
    return m_eglSurfaceQueue.empty();    
}
void HJOGRenderEnv::priCoreDone()
{
	HJFLogi("{}, renderThread priCoreDone enter", m_insName);
	if (m_eglCore)
	{
#if 0
        for (HJOGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin(); it != m_renderWindowBridgeQueue.end(); ++it)
        {
            (*it)->done();
        }
        m_renderWindowBridgeQueue.clear();
#endif
        for (HJOGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); it++)
        {
            m_eglCore->EGLSurfaceRelease((*it)->getEGLSurface());    
        }
        m_eglSurfaceQueue.clear();
        
        HJFLogi("{}, renderThread m_eglCore done enter", m_insName);
		m_eglCore->done();
        HJFLogi("{}, renderThread m_eglCore done end", m_insName);
		m_eglCore = nullptr;
	}
}

int HJOGRenderEnv::priCoreInit()
{
    int i_err = 0;
    do
    {
        HJFLogi("{} renderThread core init enter", m_insName);
        m_eglCore = std::make_shared<HJOGEGLCore>();
        if (!m_eglCore)
        {
            i_err = -1; 
            HJFLoge("{}, renderThread create egl core failed", m_insName);
            break;
        }
        m_eglCore->setInsName(m_insName + "_eglCore");
        i_err = m_eglCore->init();
        if (i_err < 0)
        {
            HJFLoge("{}, renderThread egl core init failed, err:{}", m_insName, i_err);
            break;
        }
        HJFLogi("{} renderThread core init end i_err:{}", m_insName, i_err);
        i_err = m_eglCore->EGLOffScreenSurfaceCreate(1, 1);
        if (i_err < 0)
        {
            HJFLoge("{}, create off screen surface failed, err:{}", m_insName, i_err);
            break;
        }
        HJFLogi("{} renderThread EGLOffScreenSurfaceCreate end", m_insName);
        i_err = m_eglCore->makeCurrent(m_eglCore->EGLGetOffScreenSurface());
        if (i_err < 0)
        {
            HJFLoge("{}, renderThread make current failed, err:{}", m_insName, i_err);
            break;
        }
    } while (false);
    return i_err;
}
int HJOGRenderEnv::init()
{
    int i_err = 0;
    do 
    {
        HJFLogi("{} activeInit not use kernel thread", m_insName);
        i_err = priCoreInit();
        if (i_err < 0)
        {
            HJFLoge("{} priCoreInit error", i_err);
            break;
        }    
    } while (false);
    return i_err;
}

void HJOGRenderEnv::setOGRenderEnvCb(HJOGRenderEnvCb i_cb)
{
    m_cb = i_cb;
}
void HJOGRenderEnv::setInsName(const std::string& insName)
{       
    m_insName = insName;
}
bool HJOGRenderEnv::priIsEglSurfaceHaveWindow(void *i_window)
{
    bool bHaveWindow = false;
    for (HJOGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
    {
        if ((*it)->getWindow() == i_window)
        {
            bHaveWindow = true;
            break;
        }
    }
    return bHaveWindow;
}
int HJOGRenderEnv::priUpdateEglSurface(const std::string& i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface>& o_eglSurface)
{
 int i_err = 0;
    do
    {
        HJFLogi("{} renderThread priUpdateEglSurface enter i_renderTargetInfo:{}", m_insName, i_renderTargetInfo);
        HJTransferRenderTargetInfo targetInfo;
        i_err = targetInfo.init(i_renderTargetInfo);
        if (i_err < 0)
        {
            HJFLoge("{} renderThread nativeWindowInfo deserial error", m_insName);
            break;
        }
        int targetWidth  = targetInfo.width;
        int targetHeight = targetInfo.height;
        int nState = targetInfo.state;    
        void *window = (void *)targetInfo.nativeWindow;      
        if (nState == HJTargetState_Destroy)
        {
            for (HJOGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
            {
                if ((*it)->getWindow() == window)
                {
                    o_eglSurface = *it;
                    
                    i_err = m_eglCore->EGLSurfaceRelease((*it)->getEGLSurface());
                    m_eglSurfaceQueue.erase(it);
                    
                    if (i_err < 0)
                    {
                        HJFLoge("{} renderThread EGLSurfaceCreate failed, err:{}", m_insName, i_err);
                        break;
                    }
                    break;
                }
            }		
        }
        else
        {    
#if defined(HarmonyOS)
			if (window)
			{
                //create and change  , change is not use core
				OH_NativeWindow_NativeWindowHandleOpt((OHNativeWindow*)window, SET_BUFFER_GEOMETRY, static_cast<int>(targetWidth), static_cast<int>(targetHeight));
			}
#endif
			if (nState == HJTargetState_Create)
			{
				if (!priIsEglSurfaceHaveWindow(window))
				{
					EGLSurface surface = m_eglCore->EGLSurfaceCreate(window);
					if (!surface)
					{
						i_err = -1;
						HJFLoge("{} EGLSurfaceCreate failed, err:{}", m_insName, i_err);
						break;
					}
					HJOGEGLSurface::Ptr surfacePtr = HJOGEGLSurface::Create();
					surfacePtr->setInsName(m_insName + "_eglsurface");
					surfacePtr->setWindow(window);
					surfacePtr->setEGLSurface(surface);
					surfacePtr->setTargetWidth(targetWidth);
					surfacePtr->setTargetHeight(targetHeight);
                    surfacePtr->setSurfaceType(targetInfo.type);
                    surfacePtr->setFps(targetInfo.fps);
                    
                    HJOGEGLCore::Wtr coreWtr = m_eglCore;
                    surfacePtr->setMakeCurrentCb([coreWtr, surface]()
                    {
                        HJOGEGLCore::Ptr core = coreWtr.lock();
                        if (core)
                        {
                            return core->makeCurrent(surface);
                        }
                        return HJ_OK;
                    });
                    surfacePtr->setSwapCb([coreWtr, surface]()
                    {
                        HJOGEGLCore::Ptr core = coreWtr.lock();
                        if (core)
                        {
                            return core->swap(surface);
                        }
                        return HJ_OK;
                    });
                    
                    o_eglSurface = surfacePtr;
                    
					m_eglSurfaceQueue.push_back(surfacePtr);
					HJFLogi("{} renderThread priUpdateEglSurface eglsurfacesize:{}", m_insName, m_eglSurfaceQueue.size());
				}
				else
				{
					HJFLogi("{} have same window not create!", m_insName);
				}
			}
            else if (nState == HJTargetState_Change)
            {
                for (HJOGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
                {
                    if ((*it)->getWindow() == window)
                    {
                        (*it)->setTargetWidth(targetWidth);
                        (*it)->setTargetHeight(targetHeight);
                    }    
                }
            }
        }    
        HJFLogi("{} renderThread priUpdateEglSurface end w:{} h:{} state:{} stateinfo:{}", m_insName, targetWidth, targetHeight, nState, priGetStateInfo(nState));
    } while (false);
    return i_err;
}
std::string HJOGRenderEnv::priGetStateInfo(int i_state)
{
    std::string str = "unknown";
    switch(i_state)
    {
    case HJTargetState_Create:
        str = "HJTargetState_Create";
        break;
    case HJTargetState_Change:
        str = "HJTargetState_Change";
        break;
    case HJTargetState_Destroy:
        str = "HJTargetState_Destroy";
        break;
    default:
        break;
    }
    return str;
}

int HJOGRenderEnv::procEglSurface(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface>& o_eglSurface)
{
    return priUpdateEglSurface(i_renderTargetInfo, o_eglSurface);
}

//const std::shared_ptr<HJOGEGLCore>& HJOGRenderEnv::getEglCore()
//{
//    return m_eglCore;
//}
//HJOGEGLSurfaceQueue& HJOGRenderEnv::getEglSurfaceQueue()
//{
//    return m_eglSurfaceQueue;
//}
int HJOGRenderEnv::priDrawEveryTarget(const HJOGEGLSurface::Ptr& i_surface, HJRenderEnvDraw i_draw)
{
	int i_err = 0;
	do
	{
        EGLSurface surface = i_surface->getEGLSurface();
		i_err = m_eglCore->makeCurrent(surface);
		if (i_err < 0)
		{
			HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
			break;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (i_draw)
		{
            HJBaseParam::Ptr param = HJBaseParam::Create();
            HJ_CatchMapSetVal(param, HJOGEGLSurface::Ptr, i_surface);
			i_err = i_draw(param);
			if (i_err < 0)
			{
				break;
			}
		}

		// HJFLogi("{} swap eglsurface:{} eglsurfacesize:{}", m_insName, size_t(i_eglSurface), m_eglSurfaceQueue.size());
		i_err = m_eglCore->swap(surface);
		if (i_err < 0)
		{
			HJFLoge("{} swap failed, err:{} surfaceName:{} type:{} w:{} h:{} window:{}", m_insName, i_err, i_surface->getInsName(), i_surface->getSurfaceType(), i_surface->getTargetWidth(), i_surface->getTargetHeight(), size_t(i_surface->getWindow()));
			break;
		}
	} while (false);
	return i_err;
}
    
int HJOGRenderEnv::testMakeOffCurrent()
{
    EGLSurface eglOffSurface = m_eglCore->EGLGetOffScreenSurface();
    return m_eglCore->makeCurrent(eglOffSurface);    
}
int HJOGRenderEnv::testMakeCurrent(EGLSurface i_surface)
{
    return m_eglCore->makeCurrent(i_surface);
} 
int HJOGRenderEnv::testSwap(EGLSurface i_surface)
{
    return m_eglCore->swap(i_surface);
}
//int HJOGRenderEnv::assignMakeCurrent(void *i_window)
//{
//    int i_err = HJ_OK;
//    do 
//    {
//        for (auto it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
//        {
//            if ((*it)->getWindow() == i_window)
//            {
//                i_err = m_eglCore->makeCurrent((*it)->getEGLSurface());
//                break;
//            }
//        }
//    } while (false);
//    return i_err;
//}
//int HJOGRenderEnv::assignSwap(void *i_window)
//{
//    int i_err = HJ_OK;
//    do 
//    {
//        for (auto it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
//        {
//            if ((*it)->getWindow() == i_window)
//            {
//                i_err = m_eglCore->swap((*it)->getEGLSurface());
//                break;
//            }
//        }        
//    } while (false);
//    return i_err;    
//}
int HJOGRenderEnv::foreachRender(int i_graphFps, HJRenderEnvUpdate i_update, HJRenderEnvDraw i_draw)
{
    int i_err = HJ_OK;
    do 
    {
        //HJFLogi("{} foreachRender enter {}", m_insName, m_renderIdx);
		EGLSurface eglOffSurface = m_eglCore->EGLGetOffScreenSurface();
		i_err = m_eglCore->makeCurrent(eglOffSurface);
		if (i_err < 0)
		{
			HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
			break;
		}

		if (i_update)
		{
            HJBaseParam::Ptr param = HJBaseParam::Create();
			i_err = i_update(param);
			if (i_err < 0)
			{
				break;
			}
		}
        
		if (m_eglSurfaceQueue.empty())
		{
			HJFLogi("{} offsurface priDrawEveryTarget use offscreen", m_insName);
            HJOGEGLSurface::Ptr surface = HJOGEGLSurface::Create();
            surface->setEGLSurface(eglOffSurface);
            surface->setSurfaceType(HJOGEGLSurfaceType_Default);
            surface->setTargetWidth(1);
            surface->setTargetHeight(1);
			i_err = priDrawEveryTarget(surface, i_draw);
			if (i_err < 0)
			{
				HJFLoge("{} priDrawEveryTarget offsurface failed, err:{}", m_insName, i_err);
				break;
			}
		}
		else
		{
			// HJFLogi("{} priDrawEveryTarget target size:{}", m_insName, m_eglSurfaceQueue.size());
			for (auto it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
			{
                int curFps = (*it)->getFps();
                if ((curFps <= 0) || (curFps > i_graphFps))
                {
                    curFps = i_graphFps;
				}
                int ratio = i_graphFps / curFps;
                if ((m_renderIdx % ratio) == 0)
                {
                    i_err = priDrawEveryTarget((*it), i_draw);
                    if (i_err < 0)
                    {
                        HJFLoge("{} priDrawEveryTarget failed, err:{}", m_insName, i_err);
                        break;
                    }
					    
                    int64_t renderIdx = (*it)->getRenderIdx();
                    if ((((*it)->getSurfaceType()) == HJOGEGLSurfaceType_EncoderPusher) && ((renderIdx % 360) == 0))
                    {
                        HJFLogi("OHCodecStat Encoder Render idx:{}", (*it)->getRenderIdx());
                    }
                    (*it)->addRenderIdx();
                }
			}
		}
    } while (false);
    //HJFLogi("{} foreachRender end idx:{}", m_insName, m_renderIdx);
    m_renderIdx++;
    return i_err;
}

#if 0
int HJOGRenderEnv::releaseRenderWindowBridge(HJOGRenderWindowBridge::Ptr i_bridge)
{
    return priRenderWindowBridgeRelease(i_bridge);
}
int HJOGRenderEnv::priRenderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge)
{
    HJFLogi("{} renderWindowBridgeRelease from deque", m_insName);
    HJOGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin();
    for (; it != m_renderWindowBridgeQueue.end(); ++it)
    {
        if ((*it).get() == i_bridge.get())
        {
            break;
        }
    }
    if (it != m_renderWindowBridgeQueue.end())
    {
        HJFLogi("{} find bridge done it", m_insName);
        (*it)->done();
        m_renderWindowBridgeQueue.erase(it);
        HJFLogi("{} find bridge done end", m_insName);
    }  
    return 0;
}

HJOGRenderWindowBridge::Ptr HJOGRenderEnv::acquireRenderWindowBridge()
{
    HJOGRenderWindowBridge::Ptr renderBridge = nullptr;
    int i_err = priCreateBridge(renderBridge);
    if (i_err < 0)
    {
        renderBridge = nullptr;
    }    
    return renderBridge;    
}

int HJOGRenderEnv::priCreateBridge(HJOGRenderWindowBridge::Ptr &renderBridge)
{
    renderBridge = HJOGRenderWindowBridge::Create();
    renderBridge->setInsName(m_insName + "_bridge");
    HJFLogi("{} renderThread renderWindowBridgeAcquire enter", m_insName);
    int ret = renderBridge->init();
    if (ret == 0)
    {
        m_renderWindowBridgeQueue.push_back(renderBridge);
    }
    else
    {
        HJFLoge("{} renderThread create render bridge failed, ret:{}", m_insName, ret);
    }
    HJFLogi("{} renderThread renderWindowBridgeAcquire end", m_insName);
    return ret;
}
HJOGRenderWindowBridgeQueue& HJOGRenderEnv::getRenderWindowBridgeQueue()
{
    return m_renderWindowBridgeQueue;
}
#endif

NS_HJ_END