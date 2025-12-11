//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"
#include "HJServerUtils.h"
#include "HJExecutor.h"
#include "HJCacheFile.h"

NS_HJ_BEGIN
class HJMediaStorageManager;
//***********************************************************************************//
typedef struct HJCacheInfo
{
	std::string rid;
	std::string url;
	int preCacheSize;
	int priority;
} HJCacheInfo;

class HJCacheManager : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJCacheManager);

	HJCacheManager(const HJServerParams& params, HJListener listener = nullptr);
	virtual ~HJCacheManager() noexcept;

	int init();
	void done();

	int preloadCacheFile(std::string rid, std::string url, int preCacheSize, int priority);
	int cancelCacheFile(std::string rid);
	void clearCache();

	void setStorageManager(const std::shared_ptr<HJMediaStorageManager>& storageManager) {
		m_storageManager = storageManager;
	}
	HJCacheFile::Ptr getCacheFile(std::string rid) {
		auto it = m_cacheFiles.find(rid);
		if (it != m_cacheFiles.end()) {
			return it->second;
		}
		return nullptr;
	}
	void closeCacheFile(std::string rid) {
		auto it = m_cacheFiles.find(rid);
		if (it != m_cacheFiles.end()) {
			// it->second->close();
			m_cacheFiles.erase(it);
		}
	}
protected:
	int checker(HJCacheInfo cacheInfo);
private:
	HJServerParams          m_params;
	HJListener				m_listener{};
	HJExecutorPool::Utr		m_pool;
	std::mutex              m_mutex;
	std::shared_ptr<HJMediaStorageManager> m_storageManager;
	std::map<std::string, HJCacheFile::Ptr> m_cacheFiles;
	// std::map<int, std::queue<HJCacheInfo>>  m_cacheInfos;
};

NS_HJ_END