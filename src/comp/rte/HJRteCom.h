#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJThreadPool.h"
#include "HJTransferInfo.h"
#include "HJRteUtils.h"
#include <memory>

#if defined(HarmonyOS)
    #include <GLES3/gl3.h>
#endif

NS_HJ_BEGIN

class HJRteCom;


class HJRteDriftInfo : public HJBaseParam
{
public:
    HJ_DEFINE_CREATE(HJRteDriftInfo);
    HJRteDriftInfo() = default;
    virtual ~HJRteDriftInfo() = default;
    
#if defined (HarmonyOS)    
    HJRteDriftInfo(GLuint i_textureId, int i_width, int i_height, HJRteTextureType i_textureType = HJRteTextureType_2D):
        m_textureId(i_textureId)
        ,m_srcWidth(i_width)
        ,m_srcHeight(i_height)
        ,m_textureType(i_textureType)
    {
        
    }
    
    //fixme after ref count, FBO cache, can memory;
    GLuint m_textureId = 0;
#endif
    
    void setVertexMat(float *i_mat)
    {
        memcpy(m_vertexMat, i_mat, sizeof(m_vertexMat));
    }
    void setTextureMat(float *i_mat)
    {
        memcpy(m_textureMat, i_mat, sizeof(m_textureMat));
    }

    HJRteTextureType m_textureType = HJRteTextureType_2D;
    int m_srcWidth = 0;
    int m_srcHeight = 0;
    float m_vertexMat[16]  = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
    float m_textureMat[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
};

class HJRteComLinkInfo : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteComLinkInfo);
    HJRteComLinkInfo();
    HJRteComLinkInfo(float i_x, float i_y, float i_width, float i_height);
    virtual ~HJRteComLinkInfo();

    std::string getInfo() const;
    
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 1.0f;
    float m_height = 1.0f;
};

class HJRteComLink : public HJBaseSharedObject
{
public:
	HJ_DEFINE_CREATE(HJRteComLink);
    HJRteComLink();
    virtual ~HJRteComLink();
    HJRteComLink(const std::shared_ptr<HJRteCom>& i_srcCom, const std::shared_ptr<HJRteCom>& i_dstCom, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);

    void setReady(bool i_ready);
    bool isReady() const;
    //void setRteCom(const std::shared_ptr<HJRteCom>& i_rteCom);
    //std::weak_ptr<HJRteCom> getRteComWtr() const;
    std::shared_ptr<HJRteCom> getSrcComPtr() const;
    std::shared_ptr<HJRteCom> getDstComPtr() const;
    std::string getSrcComName() const;
    std::string getDstComName() const;
    std::string getInfo() const;
    const std::shared_ptr<HJRteComLinkInfo>& getLinkInfo() const
    {
        return m_linkInfo;
    }

    int breakLink();

private:
    void priMemoryStat();
    static std::atomic<int> m_memoryRteComLinkStatIdx;

    std::shared_ptr<HJRteComLinkInfo> m_linkInfo = nullptr;
    bool m_bReady = false;
    std::weak_ptr<HJRteCom> m_srcCom;
    std::weak_ptr<HJRteCom> m_dstCom;
};

class HJRteCom : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteCom);

    HJRteCom();
    virtual ~HJRteCom();

    virtual int init(HJBaseParam::Ptr i_param);
    virtual void done();
    
    virtual int update(HJBaseParam::Ptr i_param);
    virtual int render(HJBaseParam::Ptr i_param);
    
    HJRteComLink::Ptr addTarget(HJRteCom::Ptr i_com, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);

    void setUsePriority(bool i_use)
    {
        m_bUsePriority = i_use;
    }
    bool isUsePriority() const
    {
        return m_bUsePriority;
    }
    int getPriority() const
    {
        return (int)m_priority;
    }

    void setPriority(HJRteComPriority i_priority)
    {
        m_priority = i_priority;
    }
    int getIndex() const
    {
        return m_curIdx;
    }
    void setNotify(HJBaseNotify i_notify)
    {
        m_notify = i_notify;
    }
    bool isAllPreReady();

    void removePre(const std::shared_ptr<HJRteComLink>& i_link);
    void removeNext(const std::shared_ptr<HJRteComLink>& i_link);
    bool isPreEmpty();
    bool isNextEmpty();
    int getPreCount();
    int getNextCount();
    HJRteComType getRteComType() const;
    bool isSource() const;
    bool isFilter() const;
    bool isTarget() const;
    //bool renderModeIsContain(int i_targetType);
    //void renderModeClear(int i_targetType);
    //void renderModeClearAll();
    //std::vector<HJTransferRenderModeInfo::Ptr>& renderModeGet(int i_targetType);
    //void renderModeAdd(int i_targetType, HJTransferRenderModeInfo::Ptr i_renderMode = nullptr);
    
    const std::deque<HJRteComLink::Wtr>& getNextQueue()
    {
        return m_nextQueue;
    }
	const std::deque<HJRteComLink::Wtr>& getPreQueue()
	{
		return m_preQueue;
	}
    void clearNext()
    {
        m_nextQueue.clear();
    }
    void clearPre()
    {
        m_preQueue.clear();
    }
    //void releaseFromPre(const std::shared_ptr<HJRteCom>& i_link);

  

    int foreachNextLink(std::function<int(const std::shared_ptr<HJRteComLink>& i_link)> i_func);
    int foreachPreLink(std::function<int(const std::shared_ptr<HJRteComLink>& i_link)> i_func);
protected:
    HJBaseNotify m_notify = nullptr;
private:
    static bool pricompare(const std::weak_ptr<HJRteComLink>& i_a, const std::weak_ptr<HJRteComLink>& i_b);
    static std::atomic<int> m_memoryRteComStatIdx;

    std::deque<HJRteComLink::Wtr> m_nextQueue;
    std::deque<HJRteComLink::Wtr> m_preQueue;
    int m_curIdx = 0;
    HJRteComPriority m_priority = HJRteComPriority_Default;
    bool m_bUsePriority = true;
    //int m_curIdx = 0;
    //HJPrioComType m_priority = HJPrioComType_None;
    //std::unordered_map<int, std::vector<HJTransferRenderModeInfo::Ptr>> m_renderMap;
};



NS_HJ_END



