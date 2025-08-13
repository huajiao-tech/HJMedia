#pragma once

#include "HJPrerequisites.h"
#include "HJDataDequeue.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"

NS_HJ_BEGIN

typedef enum SLComSendType
{
    SEND_TO_PRE  = 0,
    SEND_TO_OWN  = 1,
    SEND_TO_NEXT = 2
} SLComSendType;

#define ComConnect(cur, next) \
    i_err = HJBaseCom::connect(cur, next); \
    if (i_err < 0) \
    { \
        break;\
    }\

class HJBaseCom : public HJBaseObject
{
public:
	
    
	HJ_DEFINE_CREATE(HJBaseCom);
    
	HJBaseCom();
	virtual ~HJBaseCom();

    static int connect(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_nextCom);
    static int replace(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_newCom = nullptr);
    static int replace(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_newCom, HJFunc i_func);
    static HJBaseCom::Ptr queryLast(HJBaseCom::Ptr i_srcCom);
    static void findExecutor(HJBaseCom::Ptr i_curCom);
    
	void setNotify(HJBaseNotify i_notify);
	void setFilterType(int i_filterType);
    int getFilterType() const;
    
    virtual HJThreadPool::Ptr getOwnThreadPtr()
    {
        return nullptr;
    }
    
	int send(HJComMediaFrame::Ptr i_frame, SLComSendType i_sendType = SEND_TO_NEXT);
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual int doSend(HJComMediaFrame::Ptr i_frame); //sync must child implement, async is not implement
	virtual void join();
	virtual void done();
    
    void resetExecutor()
    {
        m_executorWeakQueue.clear();
    }
    HJBaseComWtrQueue& getNextComQueue()
    {
        return m_nextWeakComQueue;
    }
    HJBaseComWtrQueue& getPreComQueue()
    {
        return m_preWeakComQueue;
    }
    HJThreadWtrQueue& getExecutorThreadWtrQueue()
    {
        return m_executorWeakQueue;
    }
    void addExecutorThreadPtr(HJThreadPool::Ptr i_thread)
    {
        m_executorWeakQueue.push_back(i_thread);
    }
    
    
protected:
    virtual int  doRenderUpdate();
    virtual int  doRenderDraw(int i_targetWidth, int i_targetHeight);
    
    HJBaseNotify m_notify = nullptr;
	//std::mutex m_com_mutex;
    HJThreadWtrQueue m_executorWeakQueue;
    HJBaseComWtrQueue m_nextWeakComQueue;  
    HJBaseComWtrQueue m_preWeakComQueue;
    
private:
    static void priSetComOwnPause(HJBaseCom::Ptr i_curCom, bool i_bPause);
    static HJBaseComWtrQueue priPauseComs(HJBaseCom::Ptr i_curCom);
    static void priResumeComs(HJBaseComWtrQueue &queue);
    static HJBaseCom::Ptr priGetNextCom(HJBaseCom::Ptr i_com);
    static HJThreadPool::Ptr priGetParentExecutor(HJBaseCom::Ptr i_com);
    void priRemoveNext(HJBaseCom::Ptr i_com);
    void priRemovePre(HJBaseCom::Ptr i_com);
	int m_filterType = HJCOM_FILTER_TYPE_NONE;
};

class SLBaseComAsync : public HJBaseCom
{
public:
	HJ_DEFINE_CREATE(SLBaseComAsync);
	SLBaseComAsync();
	virtual ~SLBaseComAsync();
    
    //send to next media queue;
	virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
   
protected:
	virtual int run();
	HJSpDataDeque<HJComMediaFrame::Ptr> m_mediaFrameDeque;
private:

};

class HJBaseComAsyncThread : public SLBaseComAsync, public HJThreadPool
{
public:
	HJ_DEFINE_CREATE(HJBaseComAsyncThread);
		
	HJBaseComAsyncThread();
	virtual ~HJBaseComAsyncThread();
        
	virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void join() override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr() override;
    
    virtual void setInsName(const std::string& i_insName) override;
protected:
	virtual int run() override;
private:
};

class HJBaseComAsyncTimerThread : public SLBaseComAsync, public HJTimerThreadPool
{
public:
	HJ_DEFINE_CREATE(HJBaseComAsyncTimerThread);
	
	HJBaseComAsyncTimerThread();
	virtual ~HJBaseComAsyncTimerThread();
    
	virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void join() override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr() override;
    
    void setInsName(const std::string& i_insName) override;
protected:
	virtual int run() override;
private:
	int m_fps = 30;
};

class HJBaseComSync : public HJBaseCom
{
public:
	HJ_DEFINE_CREATE(HJBaseComSync);
		
	HJBaseComSync();
	virtual ~HJBaseComSync();

	virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
private:
};

class HJBaseRenderCom : public HJBaseComSync
{
public:
	HJ_DEFINE_CREATE(HJBaseRenderCom);
		
	HJBaseRenderCom();
	virtual ~HJBaseRenderCom();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    int renderUpdate(SLComSendType i_sendType = SEND_TO_NEXT);
    int renderDraw(int i_targetWidth, int i_targetHeight, SLComSendType i_sendType = SEND_TO_NEXT);
    
protected:
    virtual int  doRenderUpdate();
    virtual int  doRenderDraw(int i_targetWidth, int i_targetHeight);
    
private:    
    
};


NS_HJ_END



