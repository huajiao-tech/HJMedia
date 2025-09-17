#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <vector>

NS_HJ_BEGIN

class HJRteCom;
class HJRteComLink;

class HJRteGraph : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteGraph);

    HJRteGraph();
    virtual ~HJRteGraph();

    int test();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    int foreachSource(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    void graphReset();

protected:
    std::deque<std::shared_ptr<HJRteCom>> m_coms;
    std::deque<std::shared_ptr<HJRteCom>> m_sources;
    std::deque<std::shared_ptr<HJRteComLink>> m_links;
private:
    int priRemoveTarget(std::shared_ptr<HJRteCom> i_com);
    int priRemoveFilter(std::shared_ptr<HJRteCom> i_com);
    int priRemoveSource(std::shared_ptr<HJRteCom> i_com);
    int priRemoveCom(std::shared_ptr<HJRteCom> i_com);

	void removeFromLinks(std::shared_ptr<HJRteComLink> i_link);
	void removeFromComs(std::shared_ptr<HJRteCom> i_com);
	void removeFromSources(std::shared_ptr<HJRteCom> i_com);

    void priReset();
    int priRender(std::shared_ptr<HJRteCom> i_com);
    int priNotifyAndTryRender(std::shared_ptr<HJRteCom> i_header);

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



