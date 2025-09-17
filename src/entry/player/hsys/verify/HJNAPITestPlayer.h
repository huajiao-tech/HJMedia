#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

#include "HJPlayerInterface.h"

#define USE_RTE 1

NS_HJ_BEGIN
   
class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphComPlayer;
class HJRteGraphProc;

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
    int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
    int priCreateRenderGraph(const HJPlayerInfo &i_playerInfo);
    int priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo);
    
#if USE_RTE
    std::shared_ptr<HJRteGraphProc> m_prioGraphProc = nullptr; 
#else
    std::shared_ptr<HJPrioGraphProc> m_prioGraphProc = nullptr; 
#endif
    std::shared_ptr<HJGraphComPlayer> m_graphPlayer = nullptr;

    static bool s_bContextInit;
    int m_renderFps = 30;
};

NS_HJ_END



