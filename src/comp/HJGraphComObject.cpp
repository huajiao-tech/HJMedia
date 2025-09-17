#pragma once

#include "HJGraphComObject.h"
#include "HJFLog.h"
#include "HJComObject.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
HJBaseGraphCom::HJBaseGraphCom()
{

}
HJBaseGraphCom::~HJBaseGraphCom()
{

}
void HJBaseGraphCom::addCom(std::shared_ptr<HJBaseCom> i_comPtr)
{
    m_comQueue.push_back(i_comPtr);
}
void HJBaseGraphCom::removeCom(std::shared_ptr<HJBaseCom> i_comPtr)
{
    for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)
    {
        if (i_comPtr.get() == (*it).get())
        {
            (*it)->done();
            m_comQueue.erase(it);
            break;
        }    
    }
}
void HJBaseGraphCom::setNotify(HJBaseNotify i_notify)
{
    m_notify = i_notify;
}

int HJBaseGraphCom::init(HJBaseParam::Ptr i_param)
{
    return 0;
}
void HJBaseGraphCom::done()
{

}
void HJBaseGraphCom::doneAllCom()
{
    for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)
    {
        HJFLogi("doneAllCom join name:{} enter", (*it)->getInsName());
        (*it)->join();
        HJFLogi("doneAllCom join name:{} end", (*it)->getInsName());
    }
    
    for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)
    {
        HJFLogi("doneAllCom done name:{} enter", (*it)->getInsName());
        (*it)->done();
        HJFLogi("doneAllCom done name:{} end", (*it)->getInsName());
    }
    m_comQueue.clear();    
}
void HJBaseGraphCom::resetComsExecutor()
{
    for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)
    {
        (*it)->resetExecutor();
    }
    for (auto it = m_comQueue.begin(); it != m_comQueue.end(); it++)
    {
        if ((*it)->getInsName() == "HJComInterleave")
        {
            int a = 1;
        }    
        HJBaseCom::findExecutor(*it);
    } 
}
//////////////////////////////////////////////////////////////////////////////////

HJBaseGraphComAsyncThread::HJBaseGraphComAsyncThread()
{
    m_threadPool = HJThreadPool::Create();
}
HJBaseGraphComAsyncThread::~HJBaseGraphComAsyncThread()
{
}
HJThreadPool::Ptr HJBaseGraphComAsyncThread::getOwnThreadPtr()
{
    return m_threadPool->getThreadPtr();
}
int HJBaseGraphComAsyncThread::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJBaseGraphCom::init(i_param);
		if (i_err < 0)
		{
			break;
		}

        m_threadPool->setRunFunc([this]()
        {
            return run();
        });
		i_err = m_threadPool->start();
		if (i_err < 0)
		{
			break;
		}

	} while (false);
	return i_err;
}

void HJBaseGraphComAsyncThread::done()
{
    m_threadPool->done();
    HJBaseGraphCom::done();
}

int HJBaseGraphComAsyncThread::run()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
HJBaseGraphComAsyncTimerThread::HJBaseGraphComAsyncTimerThread()
{
    m_threadTimer = HJTimerThreadPool::Create();
}
HJBaseGraphComAsyncTimerThread::~HJBaseGraphComAsyncTimerThread()
{
    
}
HJThreadPool::Ptr HJBaseGraphComAsyncTimerThread::getOwnThreadPtr()
{
    return m_threadTimer->getThreadPtr();
}
int HJBaseGraphComAsyncTimerThread::getFps() const
{
    return m_fps;
}
int HJBaseGraphComAsyncTimerThread::init(HJBaseParam::Ptr i_param) 
{
    int i_err = 0;
	do
	{
		i_err = HJBaseGraphCom::init(i_param);
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
        
		i_err = m_threadTimer->startTimer(1000 / m_fps, [this]()
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
void HJBaseGraphComAsyncTimerThread::done()
{
    m_threadTimer->stopTimer();
    HJBaseGraphCom::done();
}
int HJBaseGraphComAsyncTimerThread::run()
{
    return 0;
}
//////////////////////////////////////////////////////////////////////////////////////
HJBaseGraphComSync::HJBaseGraphComSync()
{
}
HJBaseGraphComSync::~HJBaseGraphComSync()
{
}
int HJBaseGraphComSync::init(HJBaseParam::Ptr i_param)
{
	int i_err = 0;
	do
	{
		i_err = HJBaseGraphCom::init(i_param);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
void HJBaseGraphComSync::done()
{
	HJBaseGraphCom::done();
}

NS_HJ_END



