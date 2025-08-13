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

#include "HJOGShaderCommon.h"
#endif

NS_HJ_BEGIN

int HJOGRenderEnv::s_asyncTastkClearId = 100;

HJOGRenderEnv::HJOGRenderEnv()
{

}
HJOGRenderEnv::~HJOGRenderEnv()
{
    HJFLogi("{}, ~HJOGRenderEnv enter", m_insName);
}
void HJOGRenderEnv::activeDone()
{
    priCoreDone();
}
void HJOGRenderEnv::done()
{
    HJFLogi("{}, done enter", m_insName);
	int i_err = m_threadPool->sync([this]()
	{
		priCoreDone();
		return 0;
	});
    
    HJFLogi("thread pool done enter");
    if (m_threadPool)
    {
        m_threadPool->clearAllMsg();
        m_threadPool->done();
        m_threadPool = nullptr;
    }    
    
    HJFLogi("{}, done end", m_insName);
}
void HJOGRenderEnv::priCoreDone()
{
	HJFLogi("{}, renderThread timer done", m_insName);
	if (m_timer)
	{
		m_timer->done();
		m_timer = nullptr;
	}
    
	HJFLogi("{}, renderThread m_eglCore done", m_insName);
	if (m_eglCore)
	{
        for (OGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin(); it != m_renderWindowBridgeQueue.end(); ++it)
        {
            (*it)->done();
        }
        m_renderWindowBridgeQueue.clear();

        for (OGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); it++)
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

int HJOGRenderEnv::priDrawEveryTarget(EGLSurface i_eglSurface, int i_targetWidth, int i_targetHeight)
{
    int i_err = 0;
    do
    {
        i_err = m_eglCore->makeCurrent(i_eglSurface);
        if (i_err < 0)
        {
            HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
            break;
        }

#if defined(HarmonyOS)
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#endif
        for (OGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin(); it != m_renderWindowBridgeQueue.end(); ++it)
        {
            i_err = (*it)->draw(i_targetWidth, i_targetHeight);
            if (i_err < 0)
            {
                HJFLoge("{} render failed, err:{}", m_insName, i_err);
                break;
            }
        }
        //HJFLogi("{} swap eglsurface:{} eglsurfacesize:{}", m_insName, size_t(i_eglSurface), m_eglSurfaceQueue.size());
        i_err = m_eglCore->swap(i_eglSurface);
        if (i_err < 0)
        {
            HJFLoge("{} swap failed, err:{} eglsurfacesize:{}", m_insName, i_err, m_eglSurfaceQueue.size());
            break;
        }
    } while (false);
    return i_err;
}

int HJOGRenderEnv::priDraw()
{
    int i_err = 0;
    do 
    {
        if (!m_eglCore)
        {
            HJFLogi("eglCore is null, not proc");
            break;
        }
        
        EGLSurface eglOffSurface = m_eglCore->EGLGetOffScreenSurface();
        i_err = m_eglCore->makeCurrent(eglOffSurface);
        if (i_err < 0)
        {
            HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
            break;
        }

        for (OGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin(); it != m_renderWindowBridgeQueue.end(); ++it)
        {
            i_err = (*it)->update();
            if (i_err < 0)
            {
                HJFLoge("{} update failed, err:{}", m_insName, i_err);
                break;
            }
            //HJFLogi("offsurface update");
        }

        if (m_eglSurfaceQueue.empty())
        {
            if (m_cb)
            {
                m_cb(OGRenderEnvMsg_NeedSurface, 0, "need surface");
            }
            HJFLogi("{} offsurface priDrawEveryTarget use offscreen", m_insName);
            i_err = priDrawEveryTarget(eglOffSurface, 1, 1);
            if (i_err < 0)
            {
                HJFLoge("{} priDrawEveryTarget offsurface failed, err:{}", m_insName, i_err);
                break;
            }
        }
        else
        {
            //HJFLogi("{} priDrawEveryTarget target size:{}", m_insName, m_eglSurfaceQueue.size());
            for (OGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
            {
                i_err = priDrawEveryTarget((*it)->getEGLSurface(), (*it)->getTargetWidth(), (*it)->getTargetHeight());
                if (i_err < 0)
                {
                    HJFLoge("{} priDrawEveryTarget failed, err:{}", m_insName, i_err);
                    break;
                }
            }
        }
    } while (false);
    return i_err;
}

void HJOGRenderEnv::priRender()
{
    m_threadPool->asyncClear([this]()
    {
        priDraw();
        return 0;
    }, s_asyncTastkClearId);
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

        if (m_renderFps > 0)
        {
            m_timer = HJThreadTimer::Create();
            //SL::HJThreadPool::Wtr threadWtr = m_threadPool;
            m_timer->startSchedule(1000 / m_renderFps, [this/*, threadWtr*/]() 
            {  
                priRender();
                return 0;
            });
            HJFLogi("{} renderThread timer start", m_insName); 
        }
    } while (false);
    return i_err;
}
int HJOGRenderEnv::activeInit()
{
    int i_err = 0;
    do 
    {
        m_renderFps = 0;
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
int HJOGRenderEnv::init(int i_renderFps)
{
    int i_err = 0;
    do
    {
        m_renderFps = i_renderFps;
        HJFLogi("{} render fps:{}", m_insName, i_renderFps);
        if (m_renderFps <= 0)
        {
            i_err = -1;
            HJFLoge("{}t, invalid render fps:{}", m_insName, m_renderFps);
            break;
        }
        m_threadPool = HJThreadPool::Create();
        i_err = m_threadPool->start();
        if (i_err)
        {
            HJFLoge("{}, start thread pool failed, err:{}", m_insName, i_err);
            break;
        }

        i_err = m_threadPool->sync([this]()
        {
            int ret = priCoreInit();
            if (ret < 0)
            {
                HJFLoge("{}, priEglCoreCreate ret:{}", m_insName, ret);
            }
            return ret;
        });
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
    for (OGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
    {
        if ((*it)->getWindow() == i_window)
        {
            bHaveWindow = true;
            break;
        }
    }
    return bHaveWindow;
}
int HJOGRenderEnv::priUpdateEglSurface(const std::string &i_renderTargetInfo)
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
            for (OGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
            {
                if ((*it)->getWindow() == window)
                {
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
                for (OGEGLSurfaceQueueIt it = m_eglSurfaceQueue.begin(); it != m_eglSurfaceQueue.end(); ++it)
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
int HJOGRenderEnv::activeRenderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge)
{
    return priRenderWindowBridgeRelease(i_bridge);
}
int HJOGRenderEnv::priRenderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge)
{
    HJFLogi("{} renderWindowBridgeRelease from deque", m_insName);
    OGRenderWindowBridgeQueueIt it = m_renderWindowBridgeQueue.begin();
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
int HJOGRenderEnv::renderWindowBridgeRelease(HJOGRenderWindowBridge::Ptr i_bridge)
{
    int i_err = 0;
    do
    {
        HJFLogi("{} renderWindowBridgeRelease enter", m_insName);
        if (!i_bridge)
        {
            break;
        }
        int i_err = m_threadPool->sync([this, i_bridge]()
        {
            return priRenderWindowBridgeRelease(i_bridge);
        });
    } while (false);
    return i_err;
}
HJOGRenderWindowBridge::Ptr HJOGRenderEnv::activeRenderWindowBridgeAcquire(const std::string &i_renderMode)
{
    HJOGRenderWindowBridge::Ptr renderBridge = nullptr;
    int i_err = priCreateBridge(i_renderMode, renderBridge);
    if (i_err < 0)
    {
        renderBridge = nullptr;
    }    
    return renderBridge;    
}
HJOGRenderWindowBridge::Ptr HJOGRenderEnv::renderWindowBridgeAcquire(const std::string &i_renderMode)
{
    HJOGRenderWindowBridge::Ptr renderBridge = nullptr;
    do
    {
        int i_err = m_threadPool->sync([this, i_renderMode, &renderBridge]()
        {
            int ret = priCreateBridge(i_renderMode, renderBridge);
            return ret;
        });
        if (i_err < 0)
        {
            HJFLoge("{} sync bridge init error", m_insName);
        }
    } while (false);
    return renderBridge;
}
int HJOGRenderEnv::priCreateBridge(const std::string &i_renderMode, HJOGRenderWindowBridge::Ptr &renderBridge)
{
    renderBridge = HJOGRenderWindowBridge::Create();
    renderBridge->setInsName(m_insName + "_bridge");
    HJFLogi("{} renderThread renderWindowBridgeAcquire enter", m_insName);
    int ret = renderBridge->init(i_renderMode);
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
int HJOGRenderEnv::eglSurfaceChanged(const std::string& i_renderTargetInfo)
{
    int i_err = 0;
    HJFLogi("{} EglSurfaceChanged enter", m_insName);
    do 
    {
		i_err = m_threadPool->sync([this, i_renderTargetInfo]()
		{
			return priUpdateEglSurface(i_renderTargetInfo);
		});
    } while (false);
    HJFLogi("{} EglSurfaceChanged end i_err:{}", m_insName, i_err);
    return i_err;
}
int HJOGRenderEnv::eglSurfaceRemove(const std::string& i_renderTargetInfo)
{
	int i_err = 0;
	HJFLogi("{} eglSurfaceRemove enter", m_insName);
	do
	{
		i_err = m_threadPool->sync([this, i_renderTargetInfo]()
		{
			return priUpdateEglSurface(i_renderTargetInfo);
		});
	} while (false);
	HJFLogi("{} eglSurfaceRemove end i_err:{}", m_insName, i_err);
	return i_err;
}
int HJOGRenderEnv::eglSurfaceAdd(const std::string &i_renderTargetInfo)
{
    int i_err = 0;
    do
    {
        HJFLogi("{} eglSurfaceAdd enter", m_insName);
        i_err = m_threadPool->sync([this, i_renderTargetInfo]()
        {
            return priUpdateEglSurface(i_renderTargetInfo);
        });
        HJFLogi("{} eglSurfaceAdd end i_err:{}", m_insName, i_err);
    } while (false);
    return i_err;
}
int HJOGRenderEnv::activeEglSurfaceProc(const std::string &i_renderTargetInfo)
{
    return priUpdateEglSurface(i_renderTargetInfo);
}
//int HJOGRenderEnv::activeEglSurfaceAdd(const std::string &i_renderTargetInfo)
//{
//    return priUpdateEglSurface(i_renderTargetInfo);
//}
//int HJOGRenderEnv::activeEglSurfaceChanged(const std::string& i_renderTargetInfo)
//{
//    return priUpdateEglSurface(i_renderTargetInfo);
//}
//int HJOGRenderEnv::activeEglSurfaceRemove(const std::string& i_renderTargetInfo)
//{
//    return priUpdateEglSurface(i_renderTargetInfo);
//}
std::shared_ptr<HJOGEGLCore> HJOGRenderEnv::activeGetEglCore()
{
    return m_eglCore;
}
OGRenderWindowBridgeQueue& HJOGRenderEnv::activeGetRenderWindowBridgeQueue()
{
    return m_renderWindowBridgeQueue;
}
OGEGLSurfaceQueue& HJOGRenderEnv::activeGetEglSurfaceQueue()
{
    return m_eglSurfaceQueue;
}
NS_HJ_END