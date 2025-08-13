#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

class HJBaseRegularProc
{
public:
	HJ_DEFINE_CREATE(HJBaseRegularProc);

	HJBaseRegularProc();
	virtual ~HJBaseRegularProc();

	using RegularFun = std::function<void()>;
	void proc(RegularFun i_fun, bool i_bConditional = false);

protected:
	virtual bool tryUpdate();

private:
	int64_t m_currentTime = -1;
};

class HJRegularProcTimer : public HJBaseRegularProc
{
public:
	HJ_DEFINE_CREATE(HJRegularProcTimer);

	HJRegularProcTimer();
	virtual ~HJRegularProcTimer();

	void setInervalTime(int64_t i_intervalTime);

protected:
	virtual bool tryUpdate() override;
private:
	int64_t m_currentTime = -1;
	int64_t m_interval = 6000;
};

class HJRegularProcFreq : public HJBaseRegularProc
{
public:
	HJ_DEFINE_CREATE(HJRegularProcFreq);
	HJRegularProcFreq();
	virtual ~HJRegularProcFreq();

	void setInervalFreq(int64_t i_intervalFreq);

protected:
	virtual bool tryUpdate() override;
private:
	int64_t m_currentFreqIdx = -1;
	int64_t m_freqInterval = 60;
};

NS_HJ_END

