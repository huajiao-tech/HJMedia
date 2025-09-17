#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

#include "HJPlayerInterface.h"
#include "HJThreadPool.h"
#include <set>

NS_HJ_BEGIN
   
class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphPlayer;
class HJThreadPool;
class HJStatContext;

class HJNAPIPlayer
{
public:
    HJ_DEFINE_CREATE(HJNAPIPlayer);
    HJNAPIPlayer();
    virtual ~HJNAPIPlayer();

    static int contextInit(const HJEntryContextInfo& i_contextInfo);
    static void preloadUrl(const std::string& url);
    
    int openPlayer(const HJPlayerInfo &i_playerInfo, HJNAPIPlayerNotify i_notify, const HJEntryStatInfo& i_statInfo);
    int setNativeWindow(void* window, int i_width, int i_height, int i_state);
    int setMute(bool i_bMute);
    bool isMute();
    void closePlayer();
    void exitPlayer();
    int64_t getDuration();
    int64_t getPosition();
    
    void openEffect(int i_effectType);
    void closeEffect(int i_effectType);
    
private:
    
    void setInsName(const std::string &i_name)
    {
        m_insName = i_name;
    }
    const std::string& getInsName() const
    {
        return m_insName;
    }
    
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
    int priCreateRenderGraph(const HJPlayerInfo &i_playerInfo);
    int priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo);
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr;
    
    std::shared_ptr<HJGraphPlayer> m_player = nullptr;
    std::shared_ptr<HJThreadPool> m_exitThread = nullptr;
    std::shared_ptr<HJStatContext> m_statCtx = nullptr;
    
    int m_renderFps = 30;
    void *m_cacheWindow = nullptr;
    HJNAPIEntryNotify m_notify = nullptr;
    
    
    static std::mutex s_playerMutex;
    static std::set<int> s_playerIdxSet;
    static std::atomic<int> s_insIdx;
    
    
    
    int m_curIdx;
    std::string m_insName = "";
    
};

NS_HJ_END