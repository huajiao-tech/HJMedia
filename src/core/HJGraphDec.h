//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJGraphBase.h"
#include "HJMediaInfo.h"
#include "HJEnvironment.h"
#include "HJNodeDemuxer.h"
#include "HJNodeVDecoder.h"
#include "HJNodeADecoder.h"

#define HJ_GRAPHDEC_STOREAGE_CAPACITY  2 * 1000            //ms

NS_HJ_BEGIN
//***********************************************************************************//
class HJGraphDec : public HJMediaInterface, public HJGraphBase
{
public:
    using Ptr = std::shared_ptr<HJGraphDec>;
    HJGraphDec(const HJEnvironment::Ptr& env);
    virtual ~HJGraphDec();
    
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    virtual int init(const HJMediaUrlVector& mediaUrls);
    virtual int start() override;
    virtual int play() override;
    virtual int pause() override;
    //virtual int resume() override;
    //virtual int flush() override;
    virtual void done() override;
    //
    virtual int buildGraph();
    virtual int seek(const HJSeekInfo::Ptr info);
public:
    const HJMediaInfo::Ptr getMediaInfo() {
        if (m_demuxer) {
            return m_demuxer->getMediaInfo();
        }
        return nullptr;
    }
    const HJMediaNode::Ptr getVOutNode() {
        return m_videoDecoder;
    }
    const HJMediaNode::Ptr getAOutNode() {
        return m_audioDecoder;
    }
    int connectVOut(const HJMediaVNode::Ptr& node);
    int connectAOut(const HJMediaVNode::Ptr& node);

    void setTimeCapacity(int64_t capacity) {
        m_nodeTimeCapacity = capacity;
    }
    const int64_t getTimeCapacity() {
        return m_nodeTimeCapacity;
    }
protected:
    HJMediaUrl::Ptr             m_mediaUrl = nullptr;
    HJMediaUrlVector            m_mediaUrls;
    HJExecutor::Ptr            m_demuxerExecutor = nullptr;
    HJExecutor::Ptr            m_decodeExecutor = nullptr;
    HJNodeDemuxer::Ptr         m_demuxer = nullptr;
    HJNodeVDecoder::Ptr        m_videoDecoder = nullptr;
    HJNodeADecoder::Ptr        m_audioDecoder = nullptr;
    int64_t                     m_nodeTimeCapacity = HJ_GRAPHDEC_STOREAGE_CAPACITY; //ms
};
using HJGraphDecVector = std::vector<HJGraphDec::Ptr>;

NS_HJ_END
