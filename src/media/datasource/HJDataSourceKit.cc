//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJDataSourceKit.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJFileUtil.h"
#include "HJMediaUtils.h"
#include "HJMediaDBManager.h"
#include "HJLocalFileManager.h"

#define HJ_MEDIA_DB_FILE "hjmedia.sqlite"

NS_HJ_BEGIN
HJ_INSTANCE(HJDataSourceKit)
//***********************************************************************************//
HJDataSourceKit::HJDataSourceKit()
{

}

HJDataSourceKit::~HJDataSourceKit()
{

}

int HJDataSourceKit::init(HJParams::Ptr opts, HJListener listener)
{
    if (m_fileManager) {
        return HJErrAlreadyExist;
    }
    m_listener = listener;
    int res = parseParams(opts);
    if (HJ_OK != res){
        HJFLoge("init local io kit failed, error:{}", res);
        return res;
    }
    HJFLogi("entry, medias_dir:{}, medias_cache_max:{}, block_size:{}, retry_max:{}", m_params.medias_dir, m_params.medias_cache_max, m_params.block_size, m_params.retry_max);
    res = setupMediaDB();
    if (HJ_OK != res) { 
        HJFLoge("init local io kit failed, error:{}", res);
        return res;
    }
    m_fileManager = HJCreates<HJLocalFileManager>(m_dbManager, m_params.medias_dir);
    m_downloads = HJCreates<HJDownloads>([&](const HJNotification::Ptr ntfy) -> int {
        switch (ntfy->getID())
        {
        case HJ_LOCALIO_NOTIFY_CACHE_START: {
            break;
        }
        default:
            break;
        }
        notify(ntfy);
        
        return HJ_OK;
    });
    res = m_downloads->start();
    if (HJ_OK != res) { 
        HJFLoge("download start failed, error:{}", res);
        return res;
    }
    HJFLogi("end");

    return res;
}
void HJDataSourceKit::done()
{
    HJFLogi("entry");
    if (m_downloads) {
        m_downloads->done();
        m_downloads = nullptr;
    }
    destroyMediaDB();
    m_fileManager = nullptr;
    HJFLogi("end");
}

int HJDataSourceKit::setCacheSize(const std::string& dir, const int size)
{
    if(!m_dbManager) {
        return HJErrNotAlready;
    }
    HJFLogi("entry, dir:{}, size:{}", dir, size);
    return checkCategory(dir, size);
}

HJAVIOContext::Ptr HJDataSourceKit::getAVIOContext(const std::string& url, const std::string& dir, const std::string& rid, int64_t timeout)
{
    auto ctx = HJCreates<HJAVIOContext>();
    int res = ctx->open(url, dir, rid, timeout);
    if (res < HJ_OK) {
        return nullptr;
    }
    return ctx;
}

// std::pair<std::string, bool> HJDataSourceKit::getLocalUrl(const std::string& url, const std::string& dir, const std::string& rid)
// {
//     if (!m_fileManager) {
//         return std::make_pair("", false);
//     }
//     // std::string local_dir = dir.empty() ? m_params.medias_dir : dir;
//     // std::string url_rid = rid.empty() ? HJMediaUtils::makeUrlRid(url) : rid;
//     // std::string category = "";
//     // if(!local_dir.empty()) {
//     //     category = HJDBCategoryInfo::makeCategoryName(local_dir);
//     // }
//     // auto localUrl = HJMediaUtils::makeLocalUrl(local_dir, url, url_rid);

//     std::string local_dir = dir.empty() ? m_params.medias_dir : dir;
//     auto xinfo = HJXIOFileInfo(url, local_dir, rid);
//     auto dbFileInfo = m_fileManager->getCompletedFile(xinfo.rid, xinfo.category);
//     if (!dbFileInfo.has_value() || dbFileInfo->local_url != xinfo.local_url) {
//         return std::make_pair(xinfo.local_url, false);
//     }
//     if(!HJFileUtil::isFileExist(xinfo.local_url)) {
//         return std::make_pair(xinfo.local_url, false);
//     }
//     return std::make_pair(xinfo.local_url, true);
// }

// std::string HJDataSourceKit::makeLocalUrl(const std::string& url, const std::string& rid, const std::string& dir)
// {
//     std::string local_dir = dir.empty() ? m_params.medias_dir : dir;
//     return HJMediaUtils::makeLocalUrl(local_dir, url, rid);
// }

std::string HJDataSourceKit::download(const std::string& url, const std::string& dir, const std::string& rid, int preCacheSize, int priority, HJListener listener)
{
    if (!m_downloads || url.empty()) {
        return "";
    }
    HJFLogi("entry, url:{}, dir:{}, rid:{}, preCacheSize:{}, priority:{}", url, dir, rid, preCacheSize, priority);
    std::string local_dir = dir.empty() ? m_params.medias_dir : dir;
    if(!HJFileUtil::isDirExist(local_dir)) {
        HJFLoge("error, local dir:{} not exist, please set Category directory first", local_dir);
        return "";
    }
    auto xinfo = HJXIOFileInfo(url, local_dir, rid);
    xinfo.preCacheSize = preCacheSize;
    xinfo.priority = priority;
    xinfo.listener = listener;
    int res = m_downloads->download(xinfo);
    if (res < HJ_OK) {
        return "";
    }
    return xinfo.rid;
}

int HJDataSourceKit::cancel(const std::string& rid, bool deleteFile)
{
    HJ_UNUSED(deleteFile);
    if (rid.empty()) {
        return HJErrInvalidParams;
    }
    if (!m_downloads) {
        return HJErrNotAlready;
    }
    HJFLogi("entry, rid:{}, deleteFile:{}", rid, deleteFile);
    return m_downloads->cancel(rid);
}

std::string HJDataSourceKit::getVersion()
{
    return HJ_VERSION;
}

int HJDataSourceKit::parseParams(const HJParams::Ptr& opts)
{
    if(opts->haveValue("cache_opts")) {
        m_params.cache_opts = opts->getValue<HJCacheOptions>("cache_opts");
    }
    if (opts->haveValue("medias_dir")) {
        m_params.medias_dir = opts->getString("medias_dir");
    }
    if (opts->haveValue("medias_cache_max")) {
        m_params.medias_cache_max = opts->getInt("medias_cache_max");
    }
    if (opts->haveValue("block_size")) {
        m_params.block_size = opts->getInt("block_size");
    }
    if (opts->haveValue("retry_max")) {
        m_params.retry_max = opts->getInt("retry_max");
    }

    if(m_params.medias_dir.empty()) {
        HJFLoge("error, medias_dir is empty");
        return
         HJErrInvalidParams;
    }
    if (!HJFileUtil::isDirExist(m_params.medias_dir)) {
        auto isOK = HJFileUtil::makeDir(m_params.medias_dir);
        if (!isOK) {
            HJFLoge("create medias dir:{} failed", m_params.medias_dir);
            return HJErrInvalidPath;
        }
    }

    //
    std::string fmt_caches = "[";
    for (const auto& cache_opt : m_params.cache_opts)
    {
        const auto& cache_dir = cache_opt.first;
        if (!HJFileUtil::isDirExist(cache_dir)) {
            auto isOK = HJFileUtil::makeDir(cache_dir);
            if (!isOK) {
                HJFLoge("create cache dir:{} failed", cache_dir);
                return HJErrInvalidPath;
            }
        }
        fmt_caches += HJFMT("{}/{} MB,", cache_dir, cache_opt.second);
    }
    fmt_caches += "]";
    HJFLogi("end, medias_dir:{}, medias_cache_max:{} MB, block_size:{}, cache_opts:{}", m_params.medias_dir, m_params.medias_cache_max, m_params.block_size, fmt_caches);
    
    return HJ_OK;
}

int HJDataSourceKit::setupMediaDB()
{
    if (m_params.medias_dir.empty()) {
        return HJErrInvalidParams;
    }
    auto t0 = HJCurrentSteadyMS();
    HJFLogi("setup media db entry");
    int res = HJ_OK;
    try {
        std::string db_file = HJUtilitys::concatenatePath(m_params.medias_dir, HJ_MEDIA_DB_FILE);
        m_dbManager = HJCreates<HJMediaDBManager>(db_file);
        m_dbManager->init();
        //
        res = checkCategory(m_params.medias_dir, m_params.medias_cache_max);
        if (HJ_OK != res) {
            HJFLoge("error, check category failed, error:{}, medias dir:{}, medias size:{}", res, m_params.medias_dir, m_params.medias_cache_max);
            return res;
        }
        for (const auto& cache_opt : m_params.cache_opts) {
            res = checkCategory(cache_opt.first, cache_opt.second);
            if (HJ_OK != res) {
                HJFLogi("error, check category failed, error:{}, cache dir:{}, cache size:{}", res, cache_opt.first, cache_opt.second);
                return res;
            }
        }
    } catch (const HJException& e) {
        HJFLoge("create storage manager failed, error:{}", e.what());
        return HJErrInvalidParams;
    }
    HJFLogi("setup storage success, cost time: {}", HJCurrentSteadyMS() - t0);
    return HJ_OK;
}
int HJDataSourceKit::destroyMediaDB()
{
    HJFLogi("destroy media db entry");
    m_dbManager = nullptr;
    return HJ_OK;
}

int HJDataSourceKit::checkCategory(const std::string& cache_dir, const int cache_size)
{
    if(cache_dir.empty() || cache_size <= 0) {
        return HJErrInvalidParams;
    }
    auto max_size = cache_size * 1024 * 1024;
    return m_dbManager->updateCategoryMaxSize(cache_dir, max_size);
}


// HJXIOFileInfo HJDataSourceKit::makeXIOFileInfo(const std::string& url, const std::string& rid, const std::string& dir)
// {
//     std::string local_dir = dir.empty() ? m_params.medias_dir : dir;
//     HJXIOFileInfo info;
//     info.url = url;
//     info.rid = rid.empty() ? HJMediaUtils::makeUrlRid(url) : rid;
//     info.local_url = HJMediaUtils::makeLocalUrl(local_dir, url, info.rid);
//     if(!local_dir.empty()) {
//         info.category = HJDBCategoryInfo::makeCategoryName(local_dir);
//     }
//     info.priority = HJ_PRIORITY_NORMAL;
//     return info;
// }


std::vector<HJDBCategoryInfo> HJDataSourceKit::getAllCategoryInfos()
{
    if(!m_dbManager) {
        return {};
    }
    return m_dbManager->getAllCategoryInfos();
}

NS_HJ_END
