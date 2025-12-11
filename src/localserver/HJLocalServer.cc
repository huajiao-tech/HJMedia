//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJLocalServer.h"
#include "HJFLog.h"
#include "HJFileUtil.h"
#include "HJException.h"
#include "HJMediaStorageManager.h"

#define HJ_SERVER_DB_FILE "hjmedia.sqlite"

NS_HJ_BEGIN
HJ_INSTANCE(HJLocalServer)
//***********************************************************************************//
HJLocalServer::HJLocalServer()
{
}

int HJLocalServer::init(HJParams::Ptr params, HJListener listener)
{
    m_listener = listener;
    if (params) {
        parseParams(params);
    }
    if (!HJFileUtil::isDirExist(m_params.cache_dir)) {
        auto isOK = HJFileUtil::create_path(m_params.cache_dir);
        if (!isOK) {
            HJFLoge("create cache dir failed, path:{}", m_params.cache_dir);
            return HJErrInvalidPath;
        }
    }

    int res = startHTTPSever();
    if(HJ_OK != res) {
        HJFLoge("start http server failed, res:{}", res);
        return res;
    }
	res = setupStorage();
    if (HJ_OK != res) {
        HJFLoge("setup storage manager failed, res:{}", res);
        return res;
    }
    return HJ_OK;
}

void HJLocalServer::done()
{
    stopHTTPSever();
    destroyStorage();
}

/**
* origin url: http://static.s3.huajiao.com/Object.access/hj-video/OGFmOWU4NmFiMGU1ZDFlNzNlODMwMzJkYmZjNGQwODMxNjdhNDdlZi5tcDQ%3d
* out url: http://127.0.0.1:8123/play?id=443f69ac6e10946801bc4f1f3a063dae&u=http://static.s3.huajiao.com/Object.access/hj-video/OGFmOWU4NmFiMGU1ZDFlNzNlODMwMzJkYmZjNGQwODMxNjdhNDdlZi5tcDQ%3d&level=1
*/
std::string HJLocalServer::getPlayUrl(std::string rid, std::string url, int level)
{
    std::string out_url = "http://127.0.0.1:" + HJ2STR(HJ_HTTP_PORT_DEFAULT) + "/play?id=" + rid + "&u=" + url + "&level=" + HJ2STR(level);
    return out_url;
}

int HJLocalServer::preloadCacheFile(std::string rid, std::string url, int preCacheSize, int level)
{


    return HJ_OK;
}

int HJLocalServer::cancelCacheFile(std::string rid)
{
    return HJ_OK;
}

void HJLocalServer::clearCache()
{

}

std::string HJLocalServer::getVersion()
{
    return HJ_VERSION;
}

int HJLocalServer::parseParams(const HJParams::Ptr& params)
{
    m_params.cache_dir = HJUtilitys::exeDir();
    if (params->haveValue("cache_dir")) {
        m_params.cache_dir = params->getString("cache_dir");
    }
    if (params->haveValue("cache_size")) {
        m_params.cache_size = params->getInt("cache_size");
    }
    if (params->haveValue("device_id")) {
        m_params.device_id = params->getString("device_id");
    }
    if (params->haveValue("mobile_enable")) {
        m_params.mobile_enable = params->getBool("mobile_enable");
    }
    //
    if (params->haveValue("port")) {
        m_params.port = params->getInt("port");
    }
    if (params->haveValue("media_dir")) {
        m_params.media_dir = params->getString("media_dir");
    }

    return HJ_OK;
}

int HJLocalServer::startHTTPSever()
{
    m_httpServer = HJCreateu<HJHTTPServer>(m_params, HJSharedFromThis());
	return m_httpServer->start();
}

int HJLocalServer::stopHTTPSever()
{
    m_httpServer = nullptr;
    return HJ_OK;
}

int HJLocalServer::setupStorage()
{
    if (m_params.cache_dir.empty()) {
        return HJErrInvalidParams;
    }
    HJFLogi("setup storage entry");
    try {
        std::string db_file = HJUtilitys::concatenatePath(m_params.cache_dir, HJ_SERVER_DB_FILE);
        m_storageManager = HJCreates<HJMediaStorageManager>(db_file, m_params.cache_dir);
        m_storageManager->init();
        //
        m_cacheManager = HJCreateu<HJCacheManager>(m_params);
        m_cacheManager->setStorageManager(m_storageManager);
    } catch (const HJException& e) {
        HJFLoge("create storage manager failed, error:{}", e.what());
        return HJErrInvalidParams;
    }
    HJFLogi("setup storage success");
    return HJ_OK;
}
int HJLocalServer::destroyStorage()
{
    HJFLogi("destroy storage entry");
    m_cacheManager = nullptr;
    m_storageManager = nullptr;
    return HJ_OK;
}

NS_HJ_END
