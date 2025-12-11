//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJCacheManager.h"
#include "HJFLog.h"
#include "HJFileUtil.h"
#include "HJMediaStorageManager.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJCacheManager::HJCacheManager(const HJServerParams& params, HJListener listener)
	: m_params(params)
	, m_listener(listener)
{
	m_pool = HJCreateu<HJExecutorPool>();
}

HJCacheManager::~HJCacheManager() noexcept
{

}

int HJCacheManager::init()
{
	HJFLogi("entry");



    return HJ_OK;
}

void HJCacheManager::done()
{
	m_pool = nullptr;
}

int HJCacheManager::preloadCacheFile(std::string rid, std::string url, int preCacheSize, int priority)
{
	if (!m_pool || url.empty()) {
		HJFLogw("rid or url is empty");
		return HJErrInvalidParams;
	}
	HJFLogi("entry");
	HJCacheInfo cacheInfo{rid, url, preCacheSize, priority};
	m_pool->commit(priority, rid, [&]() {
		checker(cacheInfo);
		return HJ_OK;
	});

	// auto task = HJCreateu<HJPreloadCacheTask>(rid, url, preCacheSize, priority);
	// task->setListener(m_listener);
	// m_pool->addTask(task);

	return HJ_OK;
}

int HJCacheManager::cancelCacheFile(std::string rid)
{
	if (!m_pool || rid.empty()) {
		HJFLogw("rid or url is empty");
		return HJErrInvalidParams;
	}
	auto ret = m_pool->cancelTask(rid);
	HJFLogi("cancel cache file:{} task running, ret:{}", rid, ret);

	return HJ_OK;
}

void HJCacheManager::clearCache()
{

}

/**
 * storage
 * local 
 * exist or dbFileSize > preCacheSize
 */
int HJCacheManager::checker(HJCacheInfo cacheInfo)
{
	HJFLogi("entry");
	HJ_AUTOU_LOCK(m_mutex);
	HJFileStatus status = HJFileStatus::NONE;
	int64_t dbFileSize = 0;
	auto category = HJServerUtils::getCategory(cacheInfo.priority);
    auto dbInfo = m_storageManager->GetFile(category, cacheInfo.rid);
    if(dbInfo.has_value()){
		 status = (HJFileStatus)(*dbInfo).status;
		 dbFileSize = (*dbInfo).size;
    }
	//
	auto cacheFile = getCacheFile(cacheInfo.rid);
	if (cacheFile || dbFileSize > cacheInfo.preCacheSize || (HJFileStatus::COMPLETED == status)) {
		HJFLogi("cache file exist, db file status:{}, dbFileSize:{}, preCacheSize:{}", (int)status, dbFileSize, cacheInfo.preCacheSize);
		return HJ_OK;
	} else 
	{
		HJFLogi("create cache file -- rid:{}, url:{}, preCacheSize:{}, priority:{}, db file status:{}", 
			cacheInfo.rid, cacheInfo.url, cacheInfo.preCacheSize, cacheInfo.priority, (int)status);

		HJUrl::Ptr murl = HJCreates<HJUrl>(cacheInfo.url, HJ_XIO_RW);
		(*murl)["rid"] = cacheInfo.rid;
		(*murl)["preCacheSize"] = cacheInfo.preCacheSize;
		(*murl)["cacheDir"] = m_params.cache_dir;
        (*murl)["category"] = category;
		if(dbInfo.has_value()) {
			(*murl)["dbInfo"] = *dbInfo;
		}
		cacheFile = HJCreateu<HJCacheFile>();
        cacheFile->setStorageManager(m_storageManager);
		int res = cacheFile->open(murl);
		if(HJ_OK != res) {
			HJFLoge("open cache file failed, res:{}", res);
			return res;
		}
		m_cacheFiles[cacheInfo.rid] = cacheFile;
	}
	return HJ_OK;
}


NS_HJ_END
