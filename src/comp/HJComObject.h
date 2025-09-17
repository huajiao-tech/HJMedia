#pragma once

#include "HJPrerequisites.h"
#include "HJDataDequeue.h"
#include "HJComUtils.h"
#include "HJTransferInfo.h"
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

class HJBaseCom : public HJBaseSharedObject
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
    
    bool isNextFull(HJBaseCom::Ptr i_curCom);

    bool isDirectCom()
    {
        return (getOwnThreadPtr() == nullptr);
    }

	int send(HJComMediaFrame::Ptr i_frame, SLComSendType i_sendType = SEND_TO_NEXT);
  
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
    void setMediaQueueMaxSize(int i_maxSize)
    {
        m_nMediaQueueMaxSize = i_maxSize;
    }
    int getMediaQueueMaxSize() const
    {
        return m_nMediaQueueMaxSize;
    }

    virtual int init(HJBaseParam::Ptr i_param);
    virtual void join();
    virtual void done();
    virtual HJThreadPool::Ptr getOwnThreadPtr()
    {
        return nullptr;
    }
    virtual bool isMediaQueueFull()
    {
        return false;
    }
    virtual int getMediaQueueSize()
    {
        return 0;
    }

protected:
    //in a general way, async is not implement, buf HJComMuxer can implement, drop video and audio non-key frame, so async is alos implement and not use final;
    //sync must derived implement
    virtual int doSend(HJComMediaFrame::Ptr i_frame);

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
    int m_nMediaQueueMaxSize = 0;
};

class SLBaseComAsync : public HJBaseCom
{
public:
	HJ_DEFINE_CREATE(SLBaseComAsync);
	SLBaseComAsync();
	virtual ~SLBaseComAsync();
   
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual bool isMediaQueueFull() override
    {
        int maxSize = getMediaQueueMaxSize();
        return (maxSize > 0) && (getMediaQueueSize() >= maxSize);
    }
    virtual int getMediaQueueSize() override
    {
        return m_mediaFrameDeque.size();
    }
protected:
    //send to next media queue;
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int run();
	HJSpDataDeque<HJComMediaFrame::Ptr> m_mediaFrameDeque;
private:
};

class HJBaseComAsyncThread : public SLBaseComAsync
{
public:
	HJ_DEFINE_CREATE(HJBaseComAsyncThread);
		
	HJBaseComAsyncThread();
	virtual ~HJBaseComAsyncThread();
      
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void join() override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr() override;
    virtual void setInsName(const std::string& i_insName) override;

    HJThreadFuncDef(m_threadPool)

protected:
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int run() override;
private:
    std::shared_ptr<HJThreadPool> m_threadPool = nullptr;
};

class HJBaseComAsyncTimerThread : public SLBaseComAsync
{
public:
	HJ_DEFINE_CREATE(HJBaseComAsyncTimerThread);
	
	HJBaseComAsyncTimerThread();
	virtual ~HJBaseComAsyncTimerThread();
    
	
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void join() override;
	virtual void done() override;
    virtual HJThreadPool::Ptr getOwnThreadPtr() override;
    
    void setInsName(const std::string& i_insName) override;
protected:
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
	virtual int run() override;
private:
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
	int m_fps = 30;
};

class HJBaseComSync : public HJBaseCom
{
public:
	HJ_DEFINE_CREATE(HJBaseComSync);
		
	HJBaseComSync();
	virtual ~HJBaseComSync();

	
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;

protected:
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
private:
};


#if 0
class HJBaseRenderCom : public HJBaseComSync
{
public:
	HJ_DEFINE_CREATE(HJBaseRenderCom);
		
	HJBaseRenderCom();
	virtual ~HJBaseRenderCom();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
       
    int renderUpdate(HJBaseParam::Ptr i_param, SLComSendType i_sendType = SEND_TO_NEXT);
    int renderDraw(HJBaseParam::Ptr i_param, SLComSendType i_sendType = SEND_TO_NEXT);
    
    bool renderModeIsContain(int i_targetType);
    void renderModeClear(int i_targetType);
    void renderModeClearAll();
    std::vector<HJTransferRenderModeInfo::Ptr>& renderModeGet(int i_targetType);
    void renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_renderMode = nullptr);
    
protected:
    virtual int  doRenderUpdate(HJBaseParam::Ptr i_param);
    virtual int  doRenderDraw(HJBaseParam::Ptr i_param);
    
private:    
    std::unordered_map<int, std::vector<HJTransferRenderModeInfo::Ptr>> m_renderMap; 
};
#endif

NS_HJ_END



