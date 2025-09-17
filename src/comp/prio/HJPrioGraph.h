#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJPrioCom.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>

NS_HJ_BEGIN

class HJBaseParam;

struct HJPrioComCompare
{
    bool operator()(const std::shared_ptr<HJPrioCom>& a, const std::shared_ptr<HJPrioCom>& b)
    {
		if (a->getPriority() != b->getPriority())
		{
			return a->getPriority() > b->getPriority();
		}
		else
		{
			return a->getIndex() > b->getIndex();
		}
    }
};

class HJPrioGraph : public HJBaseSharedObject
{
public:
    using HJPrioQueue = std::priority_queue<std::shared_ptr<HJPrioCom>, std::vector<std::shared_ptr<HJPrioCom>>, HJPrioComCompare>;

    HJ_DEFINE_CREATE(HJPrioGraph);

    HJPrioGraph();
    virtual ~HJPrioGraph();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    int insert(std::shared_ptr<HJPrioCom>  i_com);
    void remove(std::shared_ptr<HJPrioCom>  i_com);
    void dump();
    int foreach(std::function<int(std::shared_ptr<HJPrioCom> i_com)> i_func);
    
    template<typename T>
    int foreachUseType(std::function<int(std::shared_ptr<HJPrioCom> i_com)> i_func)
    {
        int i_err = HJ_OK;
        std::deque<std::shared_ptr<T>> values;
        std::shared_ptr<T> val = nullptr;
        for (const auto& obj : m_comQueue) 
        {
            val = std::dynamic_pointer_cast<T>(obj);
            if (val)
            {
                values.push_back(val);
            }
        }
        for (auto val : values)
        {
            i_err = i_func(val);
        }    
        return i_err;
    }
    
protected:

private:
    bool priContains(std::shared_ptr<HJPrioCom>  i_com);

    HJPrioQueue m_prioQueue;
    std::deque<std::shared_ptr<HJPrioCom>> m_comQueue;
};

class HJPrioGraphTimer : public HJPrioGraph
{
public:
    HJ_DEFINE_CREATE(HJPrioGraphTimer);

    HJPrioGraphTimer();
    virtual ~HJPrioGraphTimer();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    int getFps() const;

    HJThreadFuncDef(m_threadTimer)
protected:
    virtual int run();
private:
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
    int m_fps = 30;
};


NS_HJ_END



