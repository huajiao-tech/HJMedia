#pragma once

#include "HJGPUToRAM.h"
#include "HJPrerequisites.h"
#include "HJComUtils.h"

#include "HJPlayerInterface.h"
#include "HJThreadPool.h"
#include <set>

#include "HJNotify.h"
#include "HJEntryBaseRender.h"
#include "HJSEIWrapper.h"

NS_HJ_BEGIN
   
class HJOGRenderWindowBridge;
class HJPrioGraphProc;
class HJGraphPlayer;
class HJThreadPool;
class HJStatContext;
class HJBaseGPUToRAM;
class HJRGBAMediaData;
class HJEventBus;

class HJNAPIPlayer : public HJEntryBaseRender
{
public:
    HJ_DEFINE_CREATE(HJNAPIPlayer);
    HJNAPIPlayer();
    virtual ~HJNAPIPlayer();

    static int contextInit(const HJEntryContextPlayerInfo& i_contextInfo);
    static void preloadUrl(const std::string& url);

    static std::string download(const std::string& url, const std::string& dir, const std::string& rid = "", int preCacheSize = -1, int priority = 0, HJListener listener = nullptr);
    static int cancelDownload(const std::string& rid);
    
    int openPlayer(const HJPlayerInfo &i_playerInfo, HJNAPIPlayerNotify i_notify, const HJEntryStatInfo& i_statInfo);
    int setNativeWindow(const std::string &i_classStyle, const std::string &i_insName, void* window, int i_width, int i_height, int i_state);
    int setNativeWindow(const std::string &i_classStyle, const std::string &i_insName, int64_t surfaceId, int i_width, int i_height, int i_state);
    int setMute(bool i_bMute);
    bool isMute();
    void closePlayer();
    void exitPlayer();
    int64_t getDuration();
    int64_t getCurrentTimestamp();
    int64_t getPosition();
    int setPause(bool i_pause);
    int setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb);
    void setSeiNotify(const std::function<void(const std::vector<HJSEIData>&, bool)>& i_notify);

    void registerHandlers(const std::shared_ptr<HJEventBus> i_bus);

    // void openEffect(int i_effectType);
    // void closeEffect(int i_effectType);
    
    // int openNativeSource(bool i_bUsePBO = true);
    // void closeNativeSource();
    // std::shared_ptr<HJRGBAMediaData> acquireNativeSource();
    // void setFaceInfo(HJFacePointsWrapper::Ptr faceInfo);
    
    // int openFaceu(const std::string &i_faceuUrl);
    // void closeFaceu();
private:
    struct PriSurfaceBindInfo {
        NativeWindow *nativeWindow;
        std::string classStyle;
        std::string insName;
    };
    
    // void setInsName(const std::string &i_name)
    // {
    //     m_insName = i_name;
    // }
    // const std::string& getInsName() const
    // {
    //     return m_insName;
    // }
    
    //int priSetWindow(void *i_window, int i_width, int i_height, int i_state, int i_type, int i_fps);
    int priCreateRenderGraph(const HJPlayerInfo &i_playerInfo);
    int priCreatePlayerGraph(const HJPlayerInfo &i_playerInfo);
    //std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    //std::shared_ptr<HJPrioGraphProc> m_graphProc = nullptr;
    
    std::shared_ptr<HJGraphPlayer> m_player = nullptr;
    std::shared_ptr<HJThreadPool> m_exitThread = nullptr;
    std::shared_ptr<HJStatContext> m_statCtx = nullptr;
    
    int m_renderFps = 30;
    void *m_cacheWindow = nullptr;
    std::string m_cacheClassStyle = "";
    std::string m_cacheInsName = "";
    std::map<int64_t, PriSurfaceBindInfo> m_nativeWindowMap;
    //HJNAPIEntryNotify m_notify = nullptr;
    
    
    static std::mutex s_playerMutex;
    static std::set<int> s_playerIdxSet;
    static std::atomic<int> s_insIdx;
    
    bool m_bManulDrive = false;
    int m_curIdx;
    std::function<void(const std::vector<HJSEIData>&, bool)> m_seiNotify = nullptr;
    // std::string m_insName = "";
    // std::shared_ptr<HJBaseGPUToRAM> m_gpuToRamPtr = nullptr;
};

NS_HJ_END
