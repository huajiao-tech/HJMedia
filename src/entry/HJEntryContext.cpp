#include "HJEntryContext.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"
#include "HJTime.h"
#include "HJUtilitys.h"
#include "HJNetManager.h"
#include "HJBaseUtils.h"

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
            
            HJFLogi("context init logvalid：{} dir:{} level:{} mode:{} filesize:{} num:{}", i_contextInfo.logIsValid, i_contextInfo.logDir, i_contextInfo.logLevel, i_contextInfo.logMode, i_contextInfo.logMaxFileSize, i_contextInfo.logMaxFileNum);
                       
            HJFLogi("HJEntryType:{}", (int)i_type);
            av_log_set_level(AV_LOG_INFO); //AV_LOG_TRACE;
            av_log_set_callback(onHandleFFLogCallback);
        
            avformat_network_init();
             

            if (i_type == HJEntryType_Player)
            {
                HJFLogi("registerAllNetworks enter");
                HJNetManager::registerAllNetworks();
                HJFLogi("registerAllNetworks end");
            }    

            m_bContextInit = true;
        }
    } while (false);
    return i_err;
    
}
void HJEntryContext::unInit()
{
    
}

void HJEntryContext::onHandleFFLogCallback(void* avcl, int level, const char *fmt, va_list vl)
{
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
}
NS_HJ_END