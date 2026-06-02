#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"

NS_HJ_BEGIN

class HJBaseCom;
class HJBaseGraphCom : public HJBaseSharedObject
{
public:
    
	HJ_DEFINE_CREATE(HJBaseGraphCom);

	HJBaseGraphCom();
	virtual ~HJBaseGraphCom();
    void addCom(std::shared_ptr<HJBaseCom> i_comPtr);
    void removeCom(std::shared_ptr<HJBaseCom> i_comPtr);
	void setNotify(HJBaseNotify i_notify);
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    
    void doneAllCom();
    virtual HJThreadPool::Ptr getOwnThreadPtr() 
    {
        return nullptr;
    }
    void resetComsExecutor();
protected:
    HJBaseComPtrQueue m_comQueue;
private:
	HJBaseNotify m_notify = nullptr;
};

class HJBaseGraphComAsyncThread : public HJBaseGraphCom
{
public:
	HJ_DEFINE_CREATE(HJBaseGraphComAsyncThread);
	HJBaseGraphComAsyncThread();
	virtual ~HJBaseGraphComAsyncThread();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr() override;

	HJThreadFuncDef(m_threadPool)

protected:
	virtual int run();

private:
	std::shared_ptr<HJThreadPool> m_threadPool = nullptr;
};

class HJBaseGraphComAsyncTimerThread : public HJBaseGraphCom 
{
public:
	HJ_DEFINE_CREATE(HJBaseGraphComAsyncTimerThread);
	HJBaseGraphComAsyncTimerThread();
	virtual ~HJBaseGraphComAsyncTimerThread();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr()override;
    
    int getFps() const;
    
	HJThreadFuncDef(m_threadTimer)

protected:
	virtual int run();
private:
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
    int m_fps = 30;
};


class HJBaseGraphComSync : public HJBaseGraphCom
{
public:
	HJ_DEFINE_CREATE(HJBaseGraphComSync);
		
	HJBaseGraphComSync();
	virtual ~HJBaseGraphComSync();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;

private:
};

NS_HJ_END



