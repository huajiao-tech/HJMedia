#pragma once

#include "HJComObject.h"
#include "HJFLog.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
HJBaseCom::HJBaseCom()
{

}
HJBaseCom::~HJBaseCom()
{
    HJFLogi("{} ~HJBaseCom {}", m_insName, size_t(this));
}

void HJBaseCom::setNotify(HJBaseNotify i_notify)
{
	m_notify = i_notify;
}

int HJBaseCom::doSend(HJComMediaFrame::Ptr i_frame)
{
	return 0;
}
void HJBaseCom::setFilterType(int i_filterType)
{
	m_filterType = i_filterType;
}
int HJBaseCom::getFilterType() const
{
	return m_filterType;
}
int HJBaseCom::init(HJBaseParam::Ptr i_param)
{
	return 0;
}
void HJBaseCom::done()
{

}
void HJBaseCom::join()
{

}

int HJBaseCom::send(HJComMediaFrame::Ptr i_frame, SLComSendType i_sendType)
{
	int i_err = 0;
	do
	{
		if (i_sendType == SEND_TO_NEXT)
		{
			for (auto it = m_nextWeakComQueue.begin(); it != m_nextWeakComQueue.end(); ++it)
			{
				HJBaseCom::Ptr com = HJBaseCom::GetPtrFromWtr(*it);
				if (com)
				{
					if ((m_filterType & HJCOM_FILTER_TYPE_VIDEO) && i_frame->IsVideo())
					{
						i_err = com->doSend(i_frame);
					}
					else if ((m_filterType & HJCOM_FILTER_TYPE_AUDIO) && i_frame->IsAudio())
					{
						i_err = com->doSend(i_frame);
					}
				}
			}
		}
		else if (i_sendType == SEND_TO_OWN)
		{
			doSend(i_frame);
		}
		else if (i_sendType == SEND_TO_PRE)
		{

		}

	} while (false);
	return i_err;
}

int HJBaseCom::connect(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_nextCom)
{
	int i_err = 0;
	do {
		if (!i_curCom || !i_nextCom)
		{
			i_err = -1;
			break;
		}
        if (!(i_curCom->getFilterType() & i_nextCom->getFilterType()))
		{
			i_err = -1;
			break;
		}
        
        i_curCom->m_nextWeakComQueue.push_back(i_nextCom); //cur->next   next
        i_nextCom->m_preWeakComQueue.push_back(i_curCom);  //cur<-next   pre
	} while (false);
	return i_err;
}
void HJBaseCom::findExecutor(HJBaseCom::Ptr i_com)
{
    if (!i_com->getExecutorThreadWtrQueue().empty())
    {
       return;
    }   
    HJThreadPool::Ptr threadPtr = i_com->getOwnThreadPtr();
    if (threadPtr)
    {
        i_com->addExecutorThreadPtr(threadPtr);
    } 
    else 
    {
        HJBaseComWtrQueue& queue = i_com->getPreComQueue();
        for (auto it = queue.begin(); it != queue.end(); it++)
        {
            HJBaseCom::Ptr com = HJBaseCom::GetPtrFromWtr(*it);
            HJThreadPool::Ptr parentExecutorPtr = priGetParentExecutor(com);
            if (parentExecutorPtr)
            {
                i_com->addExecutorThreadPtr(parentExecutorPtr);
            }     
        }    
//        HJThreadPool::Ptr parentExecutorPtr = priGetParentExecutor(i_com);
//        if (parentExecutorPtr)
//        {
//           i_com->addExecutorThreadPtr(parentExecutorPtr);
//        }    
    }    
}
HJThreadPool::Ptr HJBaseCom::priGetParentExecutor(HJBaseCom::Ptr i_head)
{
    if (!i_head)
    {
        return nullptr;
    }
    HJThreadPool::Ptr threadPtr = i_head->getOwnThreadPtr();
    if (threadPtr)
    {
        return threadPtr;
    }
    else 
    {
        HJBaseComWtrQueue& queue = i_head->getPreComQueue();
        for (auto it = queue.begin(); it != queue.end(); it++)
        {
            HJBaseCom::Ptr com = HJBaseCom::GetPtrFromWtr(*it);
            return priGetParentExecutor(com);
        } 
    }
}
void HJBaseCom::priRemoveNext(HJBaseCom::Ptr i_com)
{
    for (auto it = m_nextWeakComQueue.begin(); it != m_nextWeakComQueue.end(); it++)
    {
        HJBaseCom::Ptr com = HJBaseCom::GetPtrFromWtr(*it);
        if (com.get() == i_com.get())
        {
            m_nextWeakComQueue.erase(it);
            break;
        }
    } 
}
void HJBaseCom::priRemovePre(HJBaseCom::Ptr i_com)
{
    for (auto it = m_preWeakComQueue.begin(); it != m_preWeakComQueue.end(); it++)
    {
        HJBaseCom::Ptr com = HJBaseCom::GetPtrFromWtr(*it);
        if (com.get() == i_com.get())
        {
            m_preWeakComQueue.erase(it);
            break;
        }
    }    
}
HJBaseCom::Ptr HJBaseCom::queryLast(HJBaseCom::Ptr i_srcCom)
{
    HJBaseCom::Ptr head = i_srcCom;
    while (head)
    {
        HJBaseCom::Ptr next = head->priGetNextCom(head);
        if (!next)
        {
            break;
        }    
        head = next;
    }   
    return head;
}
HJBaseCom::Ptr HJBaseCom::priGetNextCom(HJBaseCom::Ptr i_com)
{
    HJBaseCom::Ptr com = nullptr;
    if (i_com)
    {
        if (!i_com->m_nextWeakComQueue.empty())
        {
            auto it = i_com->m_nextWeakComQueue.begin();
            com = HJBaseCom::GetPtrFromWtr(*it);
        }
    }    
    return com;
}

void HJBaseCom::priSetComOwnPause(HJBaseCom::Ptr i_curCom, bool i_bPause)
{
    HJThreadWtrQueue& threadQueue = i_curCom->getExecutorThreadWtrQueue();
    for (auto it = threadQueue.begin(); it != threadQueue.end(); it++)
    {
        HJThreadPool::Ptr threadPtr = HJThreadPool::GetPtrFromWtr(*it);
        if (threadPtr)
        {
            threadPtr->setPause(i_bPause);
        }    
    }
}   

int HJBaseCom::replace(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_newCom, HJFunc i_func)
{
    int i_err = 0;
    do 
    {
        HJFLogi("replace enter");
        HJBaseComWtrQueue cacheQueue = priPauseComs(i_curCom);
        if (i_func)
        {
            i_func();
        }
        HJFLogi("replace kernal enter");
        replace(i_curCom, i_newCom);
        HJFLogi("replace kernal end");
        
        priResumeComs(cacheQueue);
               
        HJFLogi("replace resume end");
        
    } while (false);
    return i_err;
}

HJBaseComWtrQueue HJBaseCom::priPauseComs(HJBaseCom::Ptr i_curCom)
{
    HJBaseComWtrQueue cacheQueue;
    
    HJFLogi("{} set own pause", i_curCom->getInsName());
    HJBaseCom::priSetComOwnPause(i_curCom, true);
    cacheQueue.push_back(i_curCom);
    
    HJBaseComWtrQueue& nextQueue = i_curCom->getNextComQueue();
    for (auto it = nextQueue.begin(); it != nextQueue.end(); it++)
    {
        HJBaseCom::Ptr comSrc = HJBaseCom::GetPtrFromWtr(*it);
        if (comSrc)
        {
            HJFLogi("{} set next {} pause", i_curCom->getInsName(), comSrc->getInsName());
            HJBaseCom::priSetComOwnPause(comSrc, true);
            cacheQueue.push_back(*it);
        }    
    }  
    
    HJBaseComWtrQueue& preQueue = i_curCom->getPreComQueue();
    for (auto it = preQueue.begin(); it != preQueue.end(); it++)
    {
        HJBaseCom::Ptr comSrc = HJBaseCom::GetPtrFromWtr(*it);
        if (comSrc)
        {
            HJFLogi("{} set pre {} pause", i_curCom->getInsName(), comSrc->getInsName());
            HJBaseCom::priSetComOwnPause(comSrc, true);
            cacheQueue.push_back(*it);
        }    
    }
    HJFLogi("{} set pause end", i_curCom->getInsName());
    
    return cacheQueue;
}
void HJBaseCom::priResumeComs(HJBaseComWtrQueue &i_queue)
{
    for (auto it = i_queue.begin(); it != i_queue.end(); it++)
    {
        HJBaseCom::Ptr comSrc = HJBaseCom::GetPtrFromWtr(*it);
        if (comSrc)
        {
            HJBaseCom::priSetComOwnPause(comSrc, false);
        }  
    }           
}

int HJBaseCom::replace(HJBaseCom::Ptr i_curCom, HJBaseCom::Ptr i_newCom)
{
    int i_err = 0;
    do 
    {
        if (!i_curCom)
        {
            i_err = -1;
            break;
        }    
        //the first src com is not suppot remove
        if (i_curCom->m_preWeakComQueue.empty())
        {
            i_err = -1;
            break;
        }    
        
        for (auto src = i_curCom->m_preWeakComQueue.begin(); src != i_curCom->m_preWeakComQueue.end(); src++)
        {
            HJBaseCom::Ptr comSrc = HJBaseCom::GetPtrFromWtr(*src);
            if (comSrc)
            {
                comSrc->priRemoveNext(i_curCom);
                if (i_newCom)
                {
                    connect(comSrc,i_newCom);
                }    
                for (auto dst = i_curCom->m_nextWeakComQueue.begin(); dst != i_curCom->m_nextWeakComQueue.end(); dst++)
                {
                    HJBaseCom::Ptr comDst = HJBaseCom::GetPtrFromWtr(*dst);
                    if (comDst)
                    {
                        comDst->priRemovePre(i_curCom);
                        if (i_newCom)
                        {
                            connect(i_newCom, comDst);
                        } 
                        else 
                        {
                            connect(comSrc, comDst);
                        }            
                    }
                }
            }
        }
    } while (false);
    return i_err;
}

int  HJBaseCom::doRenderUpdate()
{
	return 0;
}

int  HJBaseCom::doRenderDraw(int i_targetWidth, int i_targetHeight)
{
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////
SLBaseComAsync::SLBaseComAsync()
{
}

SLBaseComAsync::~SLBaseComAsync()
{
}

int SLBaseComAsync::doSend(HJComMediaFrame::Ptr i_frame)
{
	m_mediaFrameDeque.push_back_move(i_frame);
	return 0;
}

int SLBaseComAsync::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJBaseCom::init(i_param);
		if (i_err < 0)
		{
			break;
		}

	} while (false);
	return i_err;
}
void SLBaseComAsync::done()
{
	HJBaseCom::done();
}
int SLBaseComAsync::run()
{
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////

HJBaseComAsyncThread::HJBaseComAsyncThread()
{
}
HJBaseComAsyncThread::~HJBaseComAsyncThread()
{
}
int HJBaseComAsyncThread::doSend(HJComMediaFrame::Ptr i_frame)
{
	int i_err = 0;
	do
	{
		i_err = SLBaseComAsync::doSend(i_frame);
		if (i_err < 0)
		{
			break;
		}
		HJThreadPool::signal();
	} while (false);
	return i_err;
}
void HJBaseComAsyncThread::setInsName(const std::string& i_insName)
{
    HJBaseCom::setInsName(i_insName);
    HJThreadPool::setThreadName(i_insName + "_thread");
    HJFLogi("{} tryreplace asyncthread setInsnamme set thread", i_insName);
}
int HJBaseComAsyncThread::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = SLBaseComAsync::init(i_param);
		if (i_err < 0)
		{
			break;
		}
        
		HJThreadPool::setRunFunc([this]()
		{
            return run();
        });
		i_err = HJThreadPool::start();
		if (i_err < 0)
		{
			break;
		}

	} while (false);
	return i_err;
}
void HJBaseComAsyncThread::join()
{
	HJThreadPool::done();
}
void HJBaseComAsyncThread::done()
{
	HJThreadPool::done();
	SLBaseComAsync::done();
}

int HJBaseComAsyncThread::run()
{
	return 0;
}
HJThreadPool::Ptr HJBaseComAsyncThread::getOwnThreadPtr()
{
    return HJThreadPool::getThreadPtr();
}
//////////////////////////////////////////////////////////////////////////////////////
HJBaseComAsyncTimerThread::HJBaseComAsyncTimerThread()
{
}
HJBaseComAsyncTimerThread::~HJBaseComAsyncTimerThread()
{

}
void HJBaseComAsyncTimerThread::setInsName(const std::string& i_insName)
{
    HJBaseCom::setInsName(i_insName);
    HJThreadPool::setThreadName(i_insName + "_thread");    
    HJFLogi("{} tryreplace timerthread setInsnamme set thread", i_insName);
}
HJThreadPool::Ptr HJBaseComAsyncTimerThread::getOwnThreadPtr()
{
    return HJThreadPool::getThreadPtr();
}
int HJBaseComAsyncTimerThread::doSend(HJComMediaFrame::Ptr i_frame)
{
	int i_err = 0;
	do
	{
		i_err = SLBaseComAsync::doSend(i_frame);
		HJTimerThreadPool::signal();
	} while (false);
	return i_err;
}
int HJBaseComAsyncTimerThread::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = SLBaseComAsync::init(i_param);
		if (i_err < 0)
		{
			break;
		}
        
		if (i_param && i_param->contains(HJBaseParam::s_paramFps))
		{
			m_fps = i_param->getValue<int>(HJBaseParam::s_paramFps);
			if (m_fps <= 0)
			{
				i_err = -1;
				break;
			}
		}
		i_err = HJTimerThreadPool::startTimer(1000 / m_fps, [this]()
			{
				return run();
			});

		if (i_err < 0)
		{
			break;
		}

	} while (false);
	return i_err;
}
void HJBaseComAsyncTimerThread::join()
{
	HJTimerThreadPool::stopTimer();
}
void HJBaseComAsyncTimerThread::done()
{
	HJTimerThreadPool::stopTimer();
	SLBaseComAsync::done();
}
int HJBaseComAsyncTimerThread::run()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

HJBaseComSync::HJBaseComSync()
{
}
HJBaseComSync::~HJBaseComSync()
{
}

int HJBaseComSync::doSend(HJComMediaFrame::Ptr i_frame)
{
	return 0;
}
int HJBaseComSync::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJBaseCom::init(i_param);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
void HJBaseComSync::done()
{

	HJBaseCom::done();
}

////////////////////////////////////////////////////////////////
HJBaseRenderCom::HJBaseRenderCom()
{
    
}
HJBaseRenderCom::~HJBaseRenderCom()
{
    
}
    
int HJBaseRenderCom::init(HJBaseParam::Ptr i_param) 
{
	int i_err = 0;
	do
	{
		i_err = HJBaseComSync::init(i_param);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;    
}
	
void HJBaseRenderCom::done() 
{
    HJBaseComSync::done();
}

int HJBaseRenderCom::renderUpdate(SLComSendType i_sendType)
{
	int i_err = 0;
	do
	{
		if (i_sendType == SEND_TO_NEXT)
		{
			for (auto it = m_nextWeakComQueue.begin(); it != m_nextWeakComQueue.end(); ++it)
			{
				HJBaseRenderCom::Ptr com = std::dynamic_pointer_cast<HJBaseRenderCom>(HJBaseCom::GetPtrFromWtr(*it));
				if (com)
				{
					i_err = com->doRenderUpdate();
                    if (i_err < 0)
                    {
                        break;
                    }
				}
			}
		}
		else if (i_sendType == SEND_TO_OWN)
		{
			i_err = doRenderUpdate();
			if (i_err < 0)
			{
				break;
			}
		}
		else if (i_sendType == SEND_TO_PRE)
		{

		}

	} while (false);
	return i_err;
}
int HJBaseRenderCom::renderDraw(int i_targetWidth, int i_targetHeight, SLComSendType i_sendType)
{
	int i_err = 0;
	do
	{
		if (i_sendType == SEND_TO_NEXT)
		{
			for (auto it = m_nextWeakComQueue.begin(); it != m_nextWeakComQueue.end(); ++it)
			{
                HJBaseRenderCom::Ptr com = std::dynamic_pointer_cast<HJBaseRenderCom>(HJBaseCom::GetPtrFromWtr(*it));
				if (com)
				{
					i_err = com->doRenderDraw(i_targetWidth, i_targetHeight);
                    if (i_err < 0)
                    {
                        break;
                    }
				}
			}
		}
		else if (i_sendType == SEND_TO_OWN)
		{
			i_err = doRenderDraw(i_targetWidth, i_targetHeight);
			if (i_err < 0)
			{
				break;
			}
		}
		else if (i_sendType == SEND_TO_PRE)
		{

		}
	} while (false);
	return i_err;
}
int HJBaseRenderCom::doRenderUpdate()
{
    return 0;
}
int HJBaseRenderCom::doRenderDraw(int i_targetWidth, int i_targetHeight)
{
    return 0;
}
NS_HJ_END



