#include "HJPrioGraphBaseEGL.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJComEvent.h"

#if defined(HarmonyOS)
    #include "HJOGRenderEnv.h"
#endif

NS_HJ_BEGIN

HJPrioGraphBaseEGL::HJPrioGraphBaseEGL()
{
	HJ_SetInsName(HJPrioGraphBaseEGL);
}
HJPrioGraphBaseEGL::~HJPrioGraphBaseEGL()
{
}

void HJPrioGraphBaseEGL::priStatFrameRate()
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
int HJPrioGraphBaseEGL::run()
{
	int i_err = 0;
	do
	{
		priStatFrameRate();
		if (!m_renderEnv)
		{
			break;
		}
#if defined(HarmonyOS)
        if (m_renderEnv->isNeedEglSurface())
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
        
        
        HJRenderEnvUpdate updateFun = ([this](HJBaseParam::Ptr i_param)
        {
            int i_err = 0;
            do
            {
				i_err = HJPrioGraph::foreach([i_param](HJPrioCom::Ptr i_com)
					{
						int ret = 0;
						do
						{
							ret = i_com->update(i_param);
							if (ret < 0)
							{
								break;
							}
						} while (false);
						return ret;			
					});
            } while (false);
            return i_err;
        });
        
        HJRenderEnvUpdate drawFun = ([this](HJBaseParam::Ptr i_param)
        {
            int i_err = 0;
            do
            {
				i_err = HJPrioGraph::foreach([i_param](HJPrioCom::Ptr i_com)
					{
						int ret = 0;
						do
						{
							ret = i_com->render(i_param);
							if (ret < 0)
							{
								break;
							}
						} while (false);
						return ret;
					});
				
            } while (false);
            return i_err;
        });
#if defined(HarmonyOS)          
        int graphFps = HJPrioGraphTimer::getFps();
        i_err = m_renderEnv->foreachRender(graphFps, updateFun, drawFun);
        if (i_err < 0)
        {
            if (m_renderListener)
            {
                m_renderListener(std::move(HJMakeNotification(HJVIDEORENDERGRAPH_EVENT_ERROR_DRAW)));
            }    
            HJFLoge("{} foreachRender err:{}", getInsName(), i_err);
            break;
        }
#endif
	} while (false);
	return i_err;
}

int HJPrioGraphBaseEGL::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJPrioGraphTimer::init(i_param);
		if (i_err < 0)
		{
			break;
		}

		i_err = HJPrioGraphTimer::sync([this, i_param]()
		{
				int i_err = 0;
				HJFLogi("{} init egl", m_insName);
				do
				{
#if defined(HarmonyOS)
                    int insIdx = 0;
                    HJ_CatchMapPlainGetVal(i_param, int, "InsIdx", insIdx);
					m_renderEnv = HJOGRenderEnv::Create(true);
					m_renderEnv->setInsName(HJFMT("HJOGRenderEnv_{}", insIdx));
					m_renderEnv->init();
					if (i_err < 0)
					{
						break;
					}
#endif
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
int HJPrioGraphBaseEGL::eglSurfaceProc(const std::string &i_renderTargetInfo)
{
	HJFLogi("{} HJGraphComVideoCapture eglSurfaceProc enter", getInsName());
	int i_err = HJPrioGraphTimer::sync([this, i_renderTargetInfo]()
		{
			int ret = 0;
			if (m_renderEnv)
			{
				HJFLogi("{} m_renderEnv eglSurfaceProc enter", getInsName());
#if defined(HarmonyOS)
				ret = m_renderEnv->procEglSurface(i_renderTargetInfo);
#endif
				HJFLogi("{} m_renderEnv eglSurfaceProc end", getInsName());
			}
			return ret;
		});
	return i_err;
}

void HJPrioGraphBaseEGL::done()
{
	HJFLogi("{} done enter", m_insName);

	int i_err = HJPrioGraphTimer::sync([this]()
	{
        if (m_renderEnv)
        {
			HJPrioGraph::foreach([](HJPrioCom::Ptr i_com)
			{
				i_com->done(); 
				HJFLogi("{} com done priority: {} index:{} ", i_com->getInsName(), i_com->getPriority(), i_com->getIndex());
				return 0;
			});

#if defined(HarmonyOS)
            m_renderEnv->done();
#endif
            m_renderEnv = nullptr;
        }
        return 0; 
    });
    
	HJPrioGraphTimer::done();
	if (i_err < 0)
	{
		HJFLoge("{} ERROR", getInsName());
	}
	HJFLogi("{} done end-------------", m_insName);
}

NS_HJ_END