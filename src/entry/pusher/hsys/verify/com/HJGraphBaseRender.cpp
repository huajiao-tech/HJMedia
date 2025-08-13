#include "HJGraphBaseRender.h"
#include "HJFLog.h"
#include "HJComVideoCapture.h"

NS_HJ_BEGIN

HJGraphBaseRender::HJGraphBaseRender()
{
	HJ_SetInsName(HJGraphBaseRender);
}
HJGraphBaseRender::~HJGraphBaseRender()
{
}
int HJGraphBaseRender::priDrawEveryTarget(EGLSurface i_eglSurface, int i_targetWidth, int i_targetHeight)
{
	int i_err = 0;
	do
	{
		if (!m_renderEnv)
		{
			break;
		}

		HJOGEGLCore::Ptr eglCore = m_renderEnv->activeGetEglCore();
		i_err = eglCore->makeCurrent(i_eglSurface);
		if (i_err < 0)
		{
			HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
			break;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (m_drawCb)
		{
			i_err = m_drawCb(i_targetWidth, i_targetHeight);
			if (i_err < 0)
			{
				break;
			}
		}

		// HJFLogi("{} swap eglsurface:{} eglsurfacesize:{}", m_insName, size_t(i_eglSurface), m_eglSurfaceQueue.size());
		i_err = eglCore->swap(i_eglSurface);
		if (i_err < 0)
		{
			HJFLoge("{} swap failed, err:{}", m_insName, i_err);
			break;
		}
	} while (false);
	return i_err;
}

void HJGraphBaseRender::priStatFrameRate()
{
	int64_t curTime = HJCurrentSteadyMS();
	if (m_statFirstTime == 0)
	{
		m_statFirstTime = curTime;
	}
	int64_t diff = curTime - m_statFirstTime;
	if (diff && (m_statIdx % 360 == 0))
	{
		HJFLogi("{} runstat idx:{} framerate:{} curDiff:{}", m_insName, m_statIdx, (m_statIdx * 1000 / diff), (curTime - m_statPreTime));
	}
	m_statIdx++;
	m_statPreTime = curTime;
}
int HJGraphBaseRender::run()
{
	int i_err = 0;
	do
	{
		priStatFrameRate();

		if (!m_renderEnv)
		{
			break;
		}

		HJOGEGLCore::Ptr eglCore = m_renderEnv->activeGetEglCore();

		EGLSurface eglOffSurface = eglCore->EGLGetOffScreenSurface();
		i_err = eglCore->makeCurrent(eglOffSurface);
		if (i_err < 0)
		{
			HJFLoge("{} make current failed, err:{} ", m_insName, i_err);
			break;
		}

		if (m_updateCb)
		{
			i_err = m_updateCb();
			if (i_err < 0)
			{
				break;
			}
		}

		OGEGLSurfaceQueue &eglSurfaceQueue = m_renderEnv->activeGetEglSurfaceQueue();
		if (eglSurfaceQueue.empty())
		{
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
			// HJFLogi("{} priDrawEveryTarget target size:{}", m_insName, m_eglSurfaceQueue.size());
			int graphFps = HJBaseGraphComAsyncTimerThread::getFps();
			for (auto it = eglSurfaceQueue.begin(); it != eglSurfaceQueue.end(); ++it)
			{
				int curFps = (*it)->getFps();
				if ((curFps <= 0) || (curFps > graphFps))
				{
					curFps = graphFps;
				}
				int ratio = graphFps / curFps;
				if ((m_renderIdx % ratio) == 0)
				{
					i_err = priDrawEveryTarget((*it)->getEGLSurface(), (*it)->getTargetWidth(), (*it)->getTargetHeight());
					if (i_err < 0)
					{
						HJFLoge("{} priDrawEveryTarget failed, err:{}", m_insName, i_err);
						break;
					}

					int64_t renderIdx = (*it)->getRenderIdx();
					if ((((*it)->getSurfaceType()) == HJOGEGLSurfaceType_Encoder) && ((renderIdx % 360) == 0))
					{
						HJFLogi("OHCodecStat Encoder Render idx:{}", (*it)->getRenderIdx());
					}
					(*it)->addRenderIdx();
				}
			}
		}
	} while (false);
    m_renderIdx++;
	return i_err;
}

int HJGraphBaseRender::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJBaseGraphComAsyncTimerThread::init(i_param);
		if (i_err < 0)
		{
			break;
		}

		i_err = HJBaseGraphComAsyncTimerThread::sync([this, i_param]()
													 {
				int i_err = 0;
				HJFLogi("{} init egl", m_insName);
				do
				{
					m_renderEnv = HJOGRenderEnv::Create(true);
					m_renderEnv->setInsName("HJOGRenderEnv");
					m_renderEnv->activeInit();
					if (i_err < 0)
					{
						break;
					}
				} while (false);
				return i_err; });
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
int HJGraphBaseRender::eglSurfaceProc(const std::string &i_renderTargetInfo)
{
	HJFLogi("HJGraphComVideoCapture eglSurfaceProc enter");
	int i_err = HJBaseGraphComAsyncTimerThread::sync([this, i_renderTargetInfo]()
													 {
			int ret = 0;
			if (m_renderEnv)
			{
                HJFLogi("m_renderEnv eglSurfaceProc enter");
				ret = m_renderEnv->activeEglSurfaceProc(i_renderTargetInfo);
                HJFLogi("m_renderEnv eglSurfaceProc end");
			}
			return ret; });
	return i_err;
}

void HJGraphBaseRender::done()
{
	HJFLogi("{} done enter", m_insName);

	int i_err = HJBaseGraphComAsyncTimerThread::sync([this]()
													 {
        if (m_renderEnv)
        {
            m_renderEnv->activeDone();
            m_renderEnv = nullptr;
        }
        return 0; });
	HJBaseGraphComAsyncTimerThread::done();
	if (i_err < 0)
	{
		HJFLoge("ERROR");
	}
	HJFLogi("{} done end-------------", m_insName);
}

NS_HJ_END