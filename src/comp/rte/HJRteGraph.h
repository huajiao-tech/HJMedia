#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include <queue>
#include <unordered_set>
#include <vector>
#include "HJRegularProc.h"
#include "HJNotify.h"

NS_HJ_BEGIN

class HJRteCom;
class HJRteComLink;
class HJRteDriftInfo;
class HJOGBaseShader;
class HJRteComLinkInfo;
class HJRteComSource;
using HJRteGraphLinkRenderCb = std::function<void(const std::string& linkId, const std::string& fromInsName, const std::string& toInsName, bool i_bOnlyCopy)>;
using HJRteGraphFrameStatCb = std::function<void(int64_t timeMs, double fps, double renderAvgMs)>;

class HJRteGraph : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteGraph);

    HJRteGraph();
    virtual ~HJRteGraph();

    // int testTopToBottom();
    // void tesetBottomToTop();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    virtual int insertFilterAfter(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_dstShader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_dstLinkInfo = nullptr);
    int foreachSource(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachTarget(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachFilter(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachAllCom(std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);
    int foreachComFromName(const std::string& i_name, std::function<int(std::shared_ptr<HJRteCom> i_com)> i_func);

    void graphReset();

    int breakLinks(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, const std::string& i_linkId = "");
    int changeLinkInfo(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, const std::shared_ptr<HJRteComLinkInfo>& i_linkInfo);
    // int removeTarget(std::shared_ptr<HJRteCom> i_com);
    // int removeFilter(std::shared_ptr<HJRteCom> i_com);
    // int removeSource(std::shared_ptr<HJRteCom> i_com);
    int removeRecursiveCom(std::shared_ptr<HJRteCom> i_com);

    int removeSingleCom(std::shared_ptr<HJRteCom> i_com, bool i_bReLink = false);

    std::deque<std::shared_ptr<HJRteCom>>& getSources() { return m_sources; }
    std::deque<std::shared_ptr<HJRteCom>>& getTargets() { return m_targets; }
    std::deque<std::shared_ptr<HJRteCom>>& getFilters() { return m_filters; }

    template<typename T>
    std::shared_ptr<T> getUseTypeCom()
    { 
        std::shared_ptr<T> val = nullptr;
        do
        {
            for (const auto& obj : m_targets) 
            {
                val = std::dynamic_pointer_cast<T>(obj);
                if (val)
                {
                    return val;
                }
            }
            for (const auto& obj : m_filters) 
            {
                val = std::dynamic_pointer_cast<T>(obj);
                if (val)
                {
                    return val;
                }
            }
            for (const auto& obj : m_sources) 
            {
                val = std::dynamic_pointer_cast<T>(obj);
                if (val)
                {
                    return val;
                }
            }
        } while (false);
        return val;
    }


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

    void forceRefreshOnce()
    {
        m_bForceUpdateOnce = true;
    }

    //int renderFromTopToBottom(std::shared_ptr<HJRteCom> i_sourceCom, std::shared_ptr<HJRteDriftInfo> i_driftInfo);
    int renderFromBottomToTop(std::shared_ptr<HJRteCom> i_com, std::shared_ptr<HJRteDriftInfo>& o_driftInfo);
    std::shared_ptr<HJRteComSource> findSourceByInsName(const std::string& i_insName) const;
    void adjustResolutionFromSourceChain(const std::shared_ptr<HJRteComSource>& i_source, int i_width, int i_height);

    void statRefCntFromTopToBottom(std::shared_ptr<HJRteCom> i_com);

    void comDone();

    bool m_bNeedStat = false;
    int64_t m_renderStatIdx = 0;    
    static int m_regularStatTime;
    HJRegularProcTimer m_regularStatTimer;
    bool m_bForceUpdateOnce = false;

    HJListener m_renderListener = nullptr;
    HJRteGraphLinkRenderCb m_linkRenderCb = nullptr;
    HJRteGraphFrameStatCb m_frameStatCb = nullptr;
private:
    int priRemoveTarget(std::shared_ptr<HJRteCom> i_com, bool i_bRecursive = true);
    int priRemoveFilter(std::shared_ptr<HJRteCom> i_com, bool i_bReLink = true);
    int priRemoveSource(std::shared_ptr<HJRteCom> i_com, bool i_bRecursive = true);
    int priRecursiveRemoveCom(std::shared_ptr<HJRteCom> i_com);

	void removeFromLinks(std::shared_ptr<HJRteComLink> i_link);
	void removeFromComs(std::shared_ptr<HJRteCom> i_com);
	//void removeFromSources(std::shared_ptr<HJRteCom> i_com);

    void priReset();
    int priRender(std::shared_ptr<HJRteCom> i_com);
    //int priRenderFromTopToBottom(std::shared_ptr<HJRteCom> i_header, std::shared_ptr<HJRteDriftInfo> i_driftInfo);
    int priRenderFromBottomToTop(std::shared_ptr<HJRteCom> i_end);
    void priAdjustResolutionFromComRecursive(const std::shared_ptr<HJRteCom>& i_com, int i_width, int i_height, std::unordered_set<HJRteCom*>& io_visited);
    bool priStatRefCntFromTopToBottom(std::shared_ptr<HJRteCom> i_com);

    bool priTargetRender(const std::shared_ptr<HJRteCom>& i_com);
    int priBind(const std::shared_ptr<HJRteCom> &i_com);
    int priUnBind(const std::shared_ptr<HJRteCom>& i_com, bool bDraw);
    int priTrySourceProc(const std::shared_ptr<HJRteCom>& i_com);
    void priSetComDrift(const std::shared_ptr<HJRteCom>& i_com, std::shared_ptr<HJRteDriftInfo>& i_driftInfo);

	void priDebugLinkFlow(const std::shared_ptr<HJRteComLink>& i_link, bool i_bOnlyCopy);
};

class HJRteGraphDrive : public HJRteGraph
{
public:
    HJ_DEFINE_CREATE(HJRteGraphDrive);

    HJRteGraphDrive();
    virtual ~HJRteGraphDrive();

    virtual int init(std::shared_ptr<HJBaseParam> i_param);
    virtual void done();

    int manualDrive();

    int getFps() const;
    bool isManulDrive() const;
    HJThreadFuncDef(m_threadTimer)
protected:
    virtual int run();
    void priCallListener(const size_t identify, const int64_t val = 0, const std::string& msg = "");
private:
    static int s_driveId;
    int priDetailRun();
    std::shared_ptr<HJTimerThreadPool> m_threadTimer = nullptr;
    int m_fps = 30;
    bool m_bManulDrive = false;
};

NS_HJ_END



