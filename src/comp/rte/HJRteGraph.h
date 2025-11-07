#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>

NS_HJ_BEGIN

class HJRteCom;
class HJRteComLink;
class HJRteDriftInfo;

class HJRteGraph : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteGraph);

    HJRteGraph();
    virtual ~HJRteGraph();

    int testTopToBottom();
    void tesetBottomToTop();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    int foreachSource(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachTarget(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachFilter(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    void graphReset();

    // int removeTarget(std::shared_ptr<HJRteCom> i_com);
    // int removeFilter(std::shared_ptr<HJRteCom> i_com);
    // int removeSource(std::shared_ptr<HJRteCom> i_com);
    int removeCom(std::shared_ptr<HJRteCom> i_com);

    std::deque<std::shared_ptr<HJRteCom>>& getSources() { return m_sources; }
    std::deque<std::shared_ptr<HJRteCom>>& getTargets() { return m_targets; }
    std::deque<std::shared_ptr<HJRteCom>>& getFilters() { return m_filters; }

    template<typename T>
    int foreachUseType(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func)
    {
        int i_err = 0;
        std::deque<std::shared_ptr<T>> values;
        std::shared_ptr<T> val = nullptr;
        for (const auto& obj : m_targets) 
        {
            val = std::dynamic_pointer_cast<T>(obj);
            if (val)
            {
                values.push_back(val);
            }
        }
        for (const auto& obj : m_filters) 
        {
            val = std::dynamic_pointer_cast<T>(obj);
            if (val)
            {
                values.push_back(val);
            }
        }
        for (const auto& obj : m_sources) 
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
    std::deque<std::shared_ptr<HJRteCom>> m_targets;
    std::deque<std::shared_ptr<HJRteCom>> m_filters;
    std::deque<std::shared_ptr<HJRteCom>> m_sources;
    std::deque<std::shared_ptr<HJRteComLink>> m_links;

    int renderFromTopToBottom(std::shared_ptr<HJRteCom> i_sourceCom, std::shared_ptr<HJRteDriftInfo> i_driftInfo);
    int renderFromBottomToTop(std::shared_ptr<HJRteCom> i_sourceCom, std::shared_ptr<HJRteDriftInfo>& o_driftInfo);

private:
    int priRemoveTarget(std::shared_ptr<HJRteCom> i_com);
    int priRemoveFilter(std::shared_ptr<HJRteCom> i_com);
    int priRemoveSource(std::shared_ptr<HJRteCom> i_com);
    int priRemoveCom(std::shared_ptr<HJRteCom> i_com);

	void removeFromLinks(std::shared_ptr<HJRteComLink> i_link);
	void removeFromComs(std::shared_ptr<HJRteCom> i_com);
	//void removeFromSources(std::shared_ptr<HJRteCom> i_com);

    void priReset();
    int priRender(std::shared_ptr<HJRteCom> i_com);
    int priRenderFromTopToBottom(std::shared_ptr<HJRteCom> i_header, std::shared_ptr<HJRteDriftInfo> i_driftInfo);
    int priRenderFromBottomToTop(std::shared_ptr<HJRteCom> i_end, std::shared_ptr<HJRteDriftInfo>& o_driftInfo);
};

class HJRteGraphTimer : public HJRteGraph
{
public:
    HJ_DEFINE_CREATE(HJRteGraphTimer);

    HJRteGraphTimer();
    virtual ~HJRteGraphTimer();

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



