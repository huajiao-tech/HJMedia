#include "HJRteGraphBaseEGL.h"
#include "HJFLog.h"
#include "HJTime.h"

#if defined(HarmonyOS)
    #include "HJOGRenderEnv.h"
#endif

NS_HJ_BEGIN

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
	if (diff && (m_statIdx % 360 == 0))
	{
		HJFLogi("{} runstat idx:{} framerate:{} curDiff:{}", m_insName, m_statIdx, (m_statIdx * 1000 / diff), (curTime - m_statPreTime));
	}
	m_statIdx++;
	m_statPreTime = curTime;
}
//int HJRteGraphBaseEGL::run()
//{
//	int i_err = 0;
//	do
//	{
//		priStatFrameRate();
//		if (!m_renderEnv)
//		{
//			break;
//		}
//     
//        HJRenderEnvUpdate updateFun = ([this](HJBaseParam::Ptr i_param)
//        {
//            int i_err = 0;
//            do
//            {
//				//i_err = HJPrioGraph::foreach([i_param](HJPrioCom::Ptr i_com)
//				//	{
//				//		int ret = 0;
//				//		do
//				//		{
//				//			ret = i_com->update(i_param);
//				//			if (ret < 0)
//				//			{
//				//				break;
//				//			}
//				//		} while (false);
//				//		return ret;			
//				//	});
//            } while (false);
//            return i_err;
//        });
//        
//        HJRenderEnvUpdate drawFun = ([this](HJBaseParam::Ptr i_param)
//        {
//            int i_err = 0;
//            do
//            {
//				//i_err = HJPrioGraph::foreach([i_param](HJPrioCom::Ptr i_com)
//				//	{
//				//		int ret = 0;
//				//		do
//				//		{
//				//			ret = i_com->render(i_param);
//				//			if (ret < 0)
//				//			{
//				//				break;
//				//			}
//				//		} while (false);
//				//		return ret;
//				//	});
//				
//            } while (false);
//            return i_err;
//        });
//#if defined(HarmonyOS)          
//        int graphFps = HJRteGraphTimer::getFps();
//        i_err = m_renderEnv->foreachRender(graphFps, updateFun, drawFun);
//        if (i_err < 0)
//        {
//            break;
//        }
//#endif
//	} while (false);
//	return i_err;
//}

int HJRteGraphBaseEGL::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJRteGraphTimer::init(i_param);
		if (i_err < 0)
		{
			break;
		}

		i_err = HJRteGraphTimer::sync([this, i_param]()
		{
				int i_err = 0;
				HJFLogi("{} init egl", m_insName);
				do
				{
#if defined(HarmonyOS)
					m_renderEnv = HJOGRenderEnv::Create(true);
					m_renderEnv->setInsName("HJOGRenderEnv");
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
int HJRteGraphBaseEGL::eglSurfaceProc(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface> & o_eglSurface)
{
	HJFLogi("HJGraphComVideoCapture eglSurfaceProc enter");
	int i_err = HJRteGraphTimer::sync([this, i_renderTargetInfo, &o_eglSurface]()
		{
			int ret = 0;
			if (m_renderEnv)
			{
				HJFLogi("m_renderEnv eglSurfaceProc enter");
#if defined(HarmonyOS)
				ret = m_renderEnv->procEglSurface(i_renderTargetInfo, o_eglSurface);
                if (ret < 0)
                {
                    return ret;
                }
#endif
				HJFLogi("m_renderEnv eglSurfaceProc end");
			}
			return ret;
		});
	return i_err;
}

void HJRteGraphBaseEGL::done()
{
	HJFLogi("{} done enter", m_insName);

	int i_err = HJRteGraphTimer::sync([this]()
	{
        if (m_renderEnv)
        {
			//HJPrioGraph::foreach([](HJPrioCom::Ptr i_com)
			//{
			//	i_com->done(); 
			//	HJFLogi("{} com done priority: {} index:{} ", i_com->getInsName(), i_com->getPriority(), i_com->getIndex());
			//	return 0;
			//});

#if defined(HarmonyOS)
            m_renderEnv->done();
#endif
            m_renderEnv = nullptr;
        }
        return 0; 
    });
    
	HJRteGraphTimer::done();
	if (i_err < 0)
	{
		HJFLoge("ERROR");
	}
	HJFLogi("{} done end-------------", m_insName);
}

NS_HJ_END