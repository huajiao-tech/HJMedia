//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJExecutor.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJLOGMODE {
    HJLOGMODE_NONE     = (1 << 0),
    HJLOGMODE_CONSOLE  = (1 << 1),
    HJLOGMODE_FILE     = (1 << 2),
    HJLOGMODE_CALLBACK = (1 << 3),
} HJLOGMODE;

typedef enum HJLOGLevel {
    HJLOGTrace = 0,
    HJLOGDebug,
    HJLOGInfo,
    HJLOGWarning,
    HJLOGError,
} HJLOGLevel;

typedef struct HJContextConfig {
    int         mThreadNum;
    bool        mIsLog;
    int         mLogLevel;
    int         mLogMode;
    std::string mLogDir;
    std::string mDumpsDir;
    std::string mResDir;
    std::string mMediaDir;
    
    HJContextConfig() {
        mThreadNum = 0;
        mIsLog = false;
        mLogLevel = HJLOGInfo;
        mLogMode = HJLOGMODE_FILE;
        mLogDir = HJUtilitys::concatenateDir(HJUtilitys::logDir(), HJUtilitys::getTimeToString());
        mDumpsDir = HJUtilitys::dumpsDir();
        mResDir = "";
        mMediaDir = "";
    }
} HJContextConfig;

class HJContext : public HJObject//, HJSingleton<HJContext>
{
public:
    using Ptr = std::shared_ptr<HJContext>;
    static HJContext& Instance();
    //
    explicit HJContext();
    virtual ~HJContext();
    int init();
    int init(const HJContextConfig& cfg);
    int done();
public:
    const std::string& getResDir() {
        return m_cfg.mResDir;
    }
    void setResDir(const std::string& resDir) {
        m_cfg.mResDir = resDir;
    }
    const std::string getIconDir() {
        return m_cfg.mResDir + "/icon/";
    }
    const std::string& getMediaDir() {
        return m_cfg.mMediaDir;
    }
    void setMediaDir(const std::string& dir) {
        m_cfg.mMediaDir = dir;
    }
private:
    static void onHandleFFLogCallback(void* avcl, int level, const char* fmt, va_list vl);
    static void onHandleSignal(int signal);
private:
    HJContextConfig                m_cfg;
    std::atomic<size_t>             m_globalIDCounter = {0};
    uint32_t                        m_timerPeriod = 1;
    
    std::deque<HJScheduler::Ptr>   m_schedulers;
    std::deque<HJTask::Ptr>        m_tasks;
};

NS_HJ_END
