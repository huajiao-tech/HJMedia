//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJDownloader.h"
#include "HJHTTPServer.h"
#include "HJCacheManager.h"
#include "HJNotify.h"

NS_HJ_BEGIN
class HJMediaStorageManager;
//***********************************************************************************//
class /*HJ_EXPORT_API*/ HJLocalServer : public HJServerDelegate
{
public:
    HJ_INSTANCE_DECL(HJLocalServer);
    HJLocalServer();

    /**
     * cache directory
     * cache size (MB)
     * device id
     * mobile enable
     */
    int init(HJParams::Ptr params, HJListener listener);
    void done();
    //
//    bool isStart();
//    int setCacheSize(int cacheSize);

//    //
    std::string getPlayUrl(std::string rid, std::string url, int level = 0);
//    //
    int preloadCacheFile(std::string rid, std::string url, int preCacheSize, int level);
    int cancelCacheFile(std::string rid);
    void clearCache();
//    //
//    int cachePersistence(std::string rid, std::string url, std::string dir);
//    int cancelCachePersistence(std::string rid, bool deleteFile);
public:
    static std::string getVersion();
    
private:
    int parseParams(const HJParams::Ptr& params);
    int startHTTPSever();
    int stopHTTPSever();
    int setupStorage();
    int destroyStorage();
    //
    virtual int onLocalServerNotify(HJNotification::Ptr ntfy) {
        return notify(ntfy);
    }
    virtual int notify(HJNotification::Ptr ntf) {
        if (m_listener) {
            return m_listener(std::move(ntf));
        }
        return HJ_OK;
    }
private:
    HJServerParams      m_params;
    HJListener          m_listener{};
    HJHTTPServer::Utr   m_httpServer{};
    std::shared_ptr<HJMediaStorageManager>  m_storageManager{};
    HJCacheManager::Utr m_cacheManager{};
};

NS_HJ_END
