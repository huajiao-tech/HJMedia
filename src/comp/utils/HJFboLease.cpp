#include "HJFboLease.h"
#include "HJFLog.h"

NS_HJ_BEGIN


HJFboLease::~HJFboLease()
{
	release();
}

HJFboLease::HJFboLease(const std::string& i_srcName, const std::shared_ptr<HJFBOCtrlPool>& i_pool, std::shared_ptr<HJOGFBOCtrl> i_fbo, bool i_bNeedStat):
    m_srcName(i_srcName)	
    , m_poolWtr(i_pool)
    , m_fbo(std::move(i_fbo))
	, m_bNeedStat(i_bNeedStat)
{

}

std::shared_ptr<HJOGFBOCtrl> HJFboLease::get() const
{
	return m_fbo;
}

void HJFboLease::release()
{
	if (m_fbo)
	{
		auto pool = m_poolWtr.lock();
		if (pool)
		{
			if (m_bNeedStat)
			{
				//HJFLogi("~FBO HJFboLease::release {}", m_srcName);
			}
			pool->recovery(std::move(m_fbo));
		}
		m_fbo = nullptr;
	}
}

NS_HJ_END

