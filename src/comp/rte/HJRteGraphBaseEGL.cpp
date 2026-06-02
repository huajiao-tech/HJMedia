#include "HJRteGraphBaseEGL.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJFBOCtrlPool.h"
#include "HJComEvent.h"

#if defined(HarmonyOS)
    #include "HJOGRenderEnv.h"
#endif

NS_HJ_BEGIN

thread_local std::shared_ptr<HJFBOCtrlPool> HJRteGraphBaseEGL::m_fbocPool = nullptr;

HJRteGraphBaseEGL::HJRteGraphBaseEGL()
{
	HJ_SetInsName(HJRteGraphBaseEGL);
}
HJRteGraphBaseEGL::~HJRteGraphBaseEGL()
{
}

void HJRteGraphBaseEGL::priStatFrameRate()
{
	int64_t curTime = HJCurrentSteadyMS();
	if (m_statFirstTime == 0)
	{
		m_statFirstTime = curTime;
	}
	int64_t diff = curTime - m_statFirstTime;
	double frameRate = 0.0;
	double renderAvgMs = 0.0;
	if (diff && (m_statRenderIdx > 0))
	{
		frameRate = static_cast<double>(m_renderStatIdx) * 1000.0 / static_cast<double>(diff);
		renderAvgMs = static_cast<double>(m_statRenderSumTime) / static_cast<double>(m_statRenderIdx);
		HJFLogi("{} runstat idx:{} framerate:{:.2f} rendIdx:{} renderAvgTime:{:.2f} curElapseTime:{}", getDebugName(), m_renderStatIdx, frameRate, m_statRenderIdx, renderAvgMs, m_statCurElapseTime);
		if (m_frameStatCb)
		{
			m_frameStatCb(curTime, frameRate, renderAvgMs);
		}
	}
	m_statPreTime = curTime;
}
int HJRteGraphBaseEGL::run()
{
	int i_err = 0;
	do
	{
		if (m_bNeedStat)
		{
			priStatFrameRate();
		}
		
		if (!m_bCanRun)
		{
			break;
		}
		
		int64_t t0 = HJCurrentSteadyMS();

#if defined(HarmonyOS)
		if (m_renderEnv && m_renderEnv->isNeedEglSurface())
		{
			if (m_renderListener && m_bNotifyNeedSurface)
			{
				m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_NEED_SURFACE)));
				m_bNotifyNeedSurface = false;
			}
		}
		else
		{
			m_bNotifyNeedSurface = true;
		}
#endif

		if (m_renderEveryStartCb)
		{
			i_err = m_renderEveryStartCb();
			if (i_err < 0)
			{
				break;
			}
		}

		i_err = runRender();
        if (i_err < 0)
        {
			HJFLoge("{} runRender error i_err:{}", getDebugName(), i_err);
            break;
        }

		if (m_renderEveryEndCb)
		{
			i_err = m_renderEveryEndCb();
			if (i_err < 0)
			{
				break;
			}
		}
		m_statRenderIdx++;
		m_statCurElapseTime = HJCurrentSteadyMS() - t0;
		m_statRenderSumTime += m_statCurElapseTime;

	} while (false);
	return i_err;
}
int HJRteGraphBaseEGL::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJRteGraphDrive::init(i_param);
		if (i_err < 0)
		{
			break;
		}
		HJ_CatchMapGetVal(i_param, HJRenderMakeCurrent, m_makeCurrentCb);
		HJ_CatchMapGetVal(i_param, HJRenderInitCb, m_renderInitCb);
		HJ_CatchMapGetVal(i_param, HJRenderEveryStartCb, m_renderEveryStartCb);
		HJ_CatchMapGetVal(i_param, HJRenderEveryEndCb, m_renderEveryEndCb);
		
		i_err = syncOverride([this, i_param]()
		{
			int i_err = 0;
			HJFLogi("{} init egl", getDebugName());

			if (m_renderInitCb)
			{
				m_renderInitCb();
			}

			if (m_makeCurrentCb)
			{
				m_makeCurrentCb(true);
			}
			do
			{
#if defined(HarmonyOS)
				m_renderEnv = HJOGRenderEnv::Create(true);
				m_renderEnv->setDebugIdx(getDebugIdx());
				m_renderEnv->setInsName("HJOGRenderEnv");
				m_renderEnv->init();
				if (i_err < 0)
				{
					break;
				}
#endif
				m_fbocPool = HJFBOCtrlPool::Create();
			} while (false);

			return i_err;
        });
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
int HJRteGraphBaseEGL::eglSurfaceProc(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface> & o_eglSurface)
{
	HJFLogi("{} HJGraphComVideoCapture eglSurfaceProc enter", getDebugName());
	int i_err = syncOverride([this, i_renderTargetInfo, &o_eglSurface]()
		{
			int ret = 0;
			if (m_renderEnv)
			{
				HJFLogi("{} m_renderEnv eglSurfaceProc enter", getDebugName());
#if defined(HarmonyOS)
				ret = m_renderEnv->procEglSurface(i_renderTargetInfo, o_eglSurface);
                if (ret < 0)
                {
                    return ret;
                }
#endif
				HJFLogi("{} m_renderEnv eglSurfaceProc end", getDebugName());
			}
			return ret;
		});
	return i_err;
}

int HJRteGraphBaseEGL::asyncClearOverride(HJThreadTaskFunc task, int i_id)
{
	return asyncClear([this, task]()
		{
			int ret = task();
			forceRefreshOnce();
			return ret;
		}, i_id);
}
int HJRteGraphBaseEGL::asyncOverride(HJThreadTaskFunc task, int64_t i_delayTime)
{
	return async([this, task]()
		{
			int ret = task();
			forceRefreshOnce();
			return ret;
		}, i_delayTime);
}
int HJRteGraphBaseEGL::syncOverride(HJThreadTaskFunc task)
{
	return sync([this, task]()
		{
			int ret = task();
			forceRefreshOnce();
			return ret;
		});
}

void HJRteGraphBaseEGL::done()
{
	HJFLogi("{} done enter", getDebugName());

	int i_err = HJRteGraphDrive::sync([this]()
	{
		setCanRun(false);
		priDoneOnGraphThread();
		HJFLogi("{} HJRteGraph comDone enter", getDebugName());
		HJRteGraph::comDone();
		HJFLogi("{} HJRteGraph comDone end", getDebugName());
		m_fbocPool = nullptr;
        if (m_renderEnv)
        {
#if defined(HarmonyOS)
            m_renderEnv->done();
#endif
            m_renderEnv = nullptr;
        }

		if (m_makeCurrentCb)
		{
			m_makeCurrentCb(false);
		}
        return 0; 
    });
    
	HJRteGraphDrive::done();
	if (i_err < 0)
	{
		HJFLoge("{} ERROR", getDebugName());
	}
	HJFLogi("{} done end-------------", getDebugName());
}

NS_HJ_END
