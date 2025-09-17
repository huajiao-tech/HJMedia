#pragma once

#include "HJPrioCom.h"
#include "HJPrioGraph.h"
#include "HJFLog.h"
#include "HJThreadPool.h"

NS_HJ_BEGIN

/////////////////////////////////////////////////////////////////////////////////////////
HJPrioGraph::HJPrioGraph()
{

}
HJPrioGraph::~HJPrioGraph()
{

}

int HJPrioGraph::init(HJBaseParam::Ptr i_param)
{
    return HJ_OK;
}
void HJPrioGraph::done()
{
    while (!m_prioQueue.empty())
    {
        m_prioQueue.pop();
    }    
    m_comQueue.clear();
}

bool HJPrioGraph::priContains(HJPrioCom::Ptr i_com)
{
    bool bContains = false;
    do
    {
        for (auto val : m_comQueue)
        {
            if (val == i_com)
            {
                bContains = true;
                break;
            }
        }
    } while (false);
    return bContains;
}
int HJPrioGraph::insert(HJPrioCom::Ptr i_com)
{
    int i_err = HJ_OK;
    do
    {
        HJFLogi("{} insert enter name:{} prioritiy:{} idx:{}", getInsName(), i_com->getInsName(), i_com->getPriority(), i_com->getIndex());

        if (priContains(i_com))
        {
            HJFLoge("{} insert duplicate com name:{} prioritiy:{} idx:{}, not proc", getInsName(), i_com->getInsName(), i_com->getPriority(), i_com->getIndex());
            i_err = HJErrInvalidParams;
            break;
        }

        m_prioQueue.push(i_com);

        HJPrioQueue tmpQueue = std::move(m_prioQueue);

        m_comQueue.clear();
        while (!tmpQueue.empty())
        {
            HJPrioCom::Ptr task = tmpQueue.top();
            tmpQueue.pop();

            m_prioQueue.push(task);
            m_comQueue.push_back(task);
        }

        dump();

    } while (false);
    return i_err;
}
void HJPrioGraph::remove(HJPrioCom::Ptr i_com)
{
    HJFLogi("{} remove enter name:{} prioritiy:{} idx:{}", getInsName(), i_com->getInsName(), i_com->getPriority(), i_com->getIndex());

    do
    {
        if (!priContains(i_com))
        {
            HJFLoge("{} remove not found, com name:{} prioritiy:{} idx:{}, not proc", getInsName(), i_com->getInsName(), i_com->getPriority(), i_com->getIndex());
            break;
        }

        HJPrioQueue tmpQueue = std::move(m_prioQueue);

        m_comQueue.clear();

        while (!tmpQueue.empty())
        {
            HJPrioCom::Ptr task = tmpQueue.top();
            tmpQueue.pop();

            if (i_com != task)
            {
                m_prioQueue.push(task);
                m_comQueue.push_back(task);
            }
        }

        dump();

    } while (false);
}
void HJPrioGraph::dump()
{
    HJFLogi("{} dump begin====", getInsName());
    for (auto com : m_comQueue)
    {
        HJFLogi("{} priority: {} index:{} ", com->getInsName(), com->getPriority(), com->getIndex());   
    }
    HJFLogi("{} dump end---------", getInsName());
}
int HJPrioGraph::foreach(std::function<int(std::shared_ptr<HJPrioCom> i_com)> i_func)
{
    int i_err = HJ_OK;
    do
    {
        for (auto com : m_comQueue)
        {
            i_err = i_func(com);
            if (i_err < 0)
            {
                break;
            }
        }
    } while (false);
    return i_err;
}
//////////////////////////////////////////////////////////////////////

HJPrioGraphTimer::HJPrioGraphTimer()
{
    m_threadTimer = HJTimerThreadPool::Create();
}
HJPrioGraphTimer::~HJPrioGraphTimer()
{

}

int HJPrioGraphTimer::init(std::shared_ptr<HJBaseParam> i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJPrioGraph::init(i_param);
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
                //startTimer not use ret, so after, thread is already run
                return run();
            });

        if (i_err < 0)
        {
            break;
        }

    } while (false);
    return i_err;
}
void HJPrioGraphTimer::done()
{
    m_threadTimer->stopTimer();
    HJPrioGraph::done();
}

int HJPrioGraphTimer::getFps() const
{
    return m_fps;
}
int HJPrioGraphTimer::run()
{
    return HJ_OK;
}


NS_HJ_END



