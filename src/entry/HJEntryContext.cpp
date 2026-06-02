#include "HJEntryContext.h"
#include "HJFLog.h"
#include "HJTime.h"
#include "HJUtilitys.h"
#include "HJCoreVersion.h"
#include "HJBaseUtils.h"
#include "HJComEvent.h"

#if !defined(HJ_RENDER_CORE) && !defined(HJ_INFERENCE_CORE)
    #include "HJFFHeaders.h"
    #if defined(HarmonyOS)
        #include "deviceinfo.h"
    #endif
    #include "HJNetManager.h"
	#include "HJDataSourceKit.h"
#endif

NS_HJ_BEGIN

bool HJEntryContext::m_bContextInit = false;

HJEntryContext::HJEntryContext()
{
    
}
HJEntryContext::~HJEntryContext()
{
    
}
int HJEntryContext::init(HJEntryType i_type, const HJEntryContextInfo& i_contextInfo)
{
    int i_err = 0;
    do 
    {
        if (!m_bContextInit)
        {
            std::string realUrl = HJBaseUtils::getRealPath(i_contextInfo.logDir);
            i_err = HJ::HJLog::Instance().init(i_contextInfo.logIsValid, realUrl, i_contextInfo.logLevel, i_contextInfo.logMode, true, i_contextInfo.logMaxFileSize, i_contextInfo.logMaxFileNum);
            if (i_err < 0)
            {
                break;
            }
            HJFLogi("version:{} context init logvalid:{} dir:{} level:{} mode:{} filesize:{} num:{} ", HJCoreVersion::getVersionDetail(), i_contextInfo.logIsValid, i_contextInfo.logDir, i_contextInfo.logLevel, i_contextInfo.logMode, i_contextInfo.logMaxFileSize, i_contextInfo.logMaxFileNum);

#if !defined(HJ_RENDER_CORE) && !defined(HJ_INFERENCE_CORE)
    #if defined(HarmonyOS)
            int currentSystemApiVersion = OH_GetSdkApiVersion();
            HJFLogi("context init currentSystemApiVersion:{}",currentSystemApiVersion);
    #endif
            HJFLogi("context init logvalid：{} dir:{} level:{} mode:{} filesize:{} num:{} ", i_contextInfo.logIsValid, i_contextInfo.logDir, i_contextInfo.logLevel, i_contextInfo.logMode, i_contextInfo.logMaxFileSize, i_contextInfo.logMaxFileNum);
            HJFLogi("HJEntryType:{}", (int)i_type);
            HJExecutorManager::setPoolSize(1);
            av_log_set_level(AV_LOG_INFO); //AV_LOG_TRACE;
            av_log_set_callback(onHandleFFLogCallback);
            avformat_network_init(); 
            if (i_type == HJEntryType_Player)
            {
				HJFLogi("HJDataSourceKit init enter");
				const HJEntryContextPlayerInfo& playerInfo = static_cast<const HJEntryContextPlayerInfo&>(i_contextInfo);
                if(!playerInfo.medias_dir.empty())
                {
                    HJParams::Ptr params = HJCreates<HJParams>();
                    (*params)["medias_dir"] = playerInfo.medias_dir;
                    (*params)["medias_cache_max"] = playerInfo.medias_cache_max;
                    HJCacheOptions cache_opts{};
                    for (auto& other_dir : playerInfo.other_dirs_options)
                    {
                        cache_opts.push_back({other_dir.first, other_dir.second});
                    }
                    if(cache_opts.size() > 0) {
                        (*params)["cache_opts"] = cache_opts;
                    }
                    (*params)["retry_max"] = playerInfo.download_retry_max;
                    HJDataSourceKit::getInstance()->init(params, nullptr);
                    HJFLogi("HJDataSourceKit init end");
                }

                HJFLogi("registerAllNetworks enter");
                HJNetManager::registerAllNetworks();
                HJFLogi("registerAllNetworks end");
            }    
#endif
            m_bContextInit = true;
        }
    } while (false);
    return i_err;
    
}
void HJEntryContext::unInit()
{
    
}
int HJEntryContext::getSystemVersion()
{
#if !defined(HJ_RENDER_CORE) && !defined(HJ_INFERENCE_CORE)
    #if defined(HarmonyOS)
        int currentSystemApiVersion = OH_GetSdkApiVersion();
        return currentSystemApiVersion;
    #else
        return 0;
    #endif
#else
    return 0;
#endif
}
void HJEntryContext::onHandleFFLogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
#if !defined(HJ_RENDER_CORE) && !defined(HJ_INFERENCE_CORE)
    switch (level) {
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
    case AV_LOG_ERROR:
    case AV_LOG_WARNING:
    case AV_LOG_INFO:
    {
        char* strp = NULL;
        int res = vasprintf(&strp, fmt, vl);
        if (res > 0 && strp) {
            HJLogi("time:" + HJ2STR(HJCurrentMicrosecond()) + ", ffmpeg: " + HJ2SSTR(strp));
            free(strp);
        }
//        char buffer[8*1024];
//#if (defined WIN32_LIB) && _MSC_VER >= 1310
//        vsnprintf(buffer, sizeof(buffer), fmt, vl);
//#else
//        vsprintf(buffer, fmt, vl);
//#endif
        //HJLogi("time:" + HJ2STR(HJUtils::getCurrentMicrosecond()) + ", ffmpeg: " + HJ2SSTR(buffer));
    }
    break;

    case AV_LOG_QUIET:

        break;
    //case AV_LOG_INFO:

    //    break;
    case AV_LOG_VERBOSE:

        break;

    case AV_LOG_DEBUG:

        break;
    default:
        break;
    }
#endif
}

int HJEntryContext::renderNotifyIdMap(int i_renderId)
{
    int type = HJVIDEORENDERGRAPH_EVENT_NONE;
	switch (i_renderId)
	{
	case HJVIDEORENDERGRAPH_EVENT_ERROR_DEFAULT:
	{
		type = HJ_RENDER_NOTIFY_ERROR_DEFAULT;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_ERROR_UPDATE:
	{
		type = HJ_RENDER_NOTIFY_ERROR_UPDATE;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_ERROR_DRAW:
	{
		type = HJ_RENDER_NOTIFY_ERROR_DRAW;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_ERROR_INIT:
	{
		type = HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_INIT_SUCCESS:
	{
		type = HJ_RENDER_NOTIFY_INIT_SUCCESS;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_INIT_FAILED:
	{
		type = HJ_RENDER_NOTIFY_INIT_ERROR;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_ERROR_PNGSEQ_INIT:
	{
		type = HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE:
	{
		type = HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NEED_SURFACE:
	{
		type = HJ_RENDER_NOTIFY_NEED_SURFACE;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_FACEU_ERROR:
	{
		type = HJ_RENDER_NOTIFY_FACEU_ERROR;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE:
	{
		type = HJ_RENDER_NOTIFY_FACEU_COMPLETE;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeCreateSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeCreateSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeCreateFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeCreateFailed;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeConnectSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeConnectSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeConnectFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeConnectFailed;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeDisconnectSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeDisconnectSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeDisconnectFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeDisconnectFailed;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeDeleteSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeDeleteSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeDeleteFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeDeleteFailed;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeEnableSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeEnableSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeEnableFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeEnableFailed;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeLinkInfoChangeSuccess:
	{
		type = HJ_RENDER_NOTIFY_NodeLinkInfoChangeSuccess;
		break;
	}
	case HJVIDEORENDERGRAPH_EVENT_NodeLinkInfoChangeFailed:
	{
		type = HJ_RENDER_NOTIFY_NodeLinkInfoChangeFailed;
		break;
	}
	default:
	{
		break;
	}
    }
    return type;
}
NS_HJ_END
