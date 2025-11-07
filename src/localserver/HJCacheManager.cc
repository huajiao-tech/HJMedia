//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJCacheManager.h"
#include "HJFLog.h"

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
	HJFLogi("entry");

	//auto task = HJCreateu<HJPreloadCacheTask>(rid, url, preCacheSize, priority);
	//task->setListener(m_listener);
	//m_pool->addTask(task);

	return HJ_OK;
}

int HJCacheManager::cancelCacheFile(std::string rid)
{
	return HJ_OK;
}

void HJCacheManager::clearCache()
{

}


NS_HJ_END