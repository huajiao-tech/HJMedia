#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

#include "HJPlayerInterface.h"

#define USE_RTE 1

NS_HJ_BEGIN
   
class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphPlayer;
class HJRteGraphProc;
class HJOGEGLSurface;

class HJNAPITestPlayer
{
public:
    HJ_DEFINE_CREATE(HJNAPITestPlayer);
    HJNAPITestPlayer();
    virtual ~HJNAPITestPlayer();

    static int contextInit(const HJEntryContextInfo& i_contextInfo);
    
    int openPlayer(const HJPlayerInfo &i_playerInfo, HJNAPIPlayerNotify i_notify);
    int setNativeWindow(void* window, int i_width, int i_height, int i_state);
    void closePlayer();
    
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
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps, std::shared_ptr<HJOGEGLSurface> & o_eglSurface);
    int priCreateRenderGraph(const HJPlayerInfo &i_playerInfo);
    int priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo);
    
#if USE_RTE
    std::shared_ptr<HJRteGraphProc> m_prioGraphProc = nullptr; 
#else
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr; 
#endif
    std::shared_ptr<HJGraphPlayer> m_player = nullptr;
    void *m_cacheWindow = nullptr;
    static bool s_bContextInit;
    int m_renderFps = 30;
    int m_curIdx;
    std::string m_insName = "";
};

NS_HJ_END



