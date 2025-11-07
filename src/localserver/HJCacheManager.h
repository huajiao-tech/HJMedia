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

NS_HJ_BEGIN
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
private:
	HJServerParams          m_params;
	HJListener				m_listener{};
	HJExecutorPool::Utr		m_pool;
	std::map<int, std::queue<HJCacheInfo>>  m_cacheInfos;
};

NS_HJ_END