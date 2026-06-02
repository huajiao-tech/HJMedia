#pragma once

#include "HJPlugin.h"
#include "HJBaseCodec.h"
#include "HJPrerequisites.h"
#include "HJSEIWrapper.h"

NS_HJ_BEGIN

class HJCodecPassThroughInfo
{
public:
    HJ_DEFINE_CREATE(HJCodecPassThroughInfo);
    HJCodecPassThroughInfo()
    {
        
    }
    virtual ~HJCodecPassThroughInfo()
    {
        
    }
    HJCodecPassThroughInfo(int64_t i_demuxSysTime, bool i_bKey, HJSEINals::Ptr i_seinals):
        m_demuxSysTime(i_demuxSysTime)
        ,m_bKey(i_bKey)
        ,m_seinals(i_seinals)
    {
        
    }
    void setDemuxSysTime(int64_t i_demuxSysTime)
    {
        m_demuxSysTime = i_demuxSysTime;        
    }
    int64_t getDemuxSysTime() const
    {
        return m_demuxSysTime;
    }
	void setKey(bool i_bKey)
	{
		m_bKey = i_bKey;
	}
	bool isKey() const
	{
		return m_bKey;
	}
    HJSEINals::Ptr getSEINals() const
    {
        return m_seinals;
    }
private:
    int64_t m_demuxSysTime = 0;
    bool m_bKey = false;
    HJSEINals::Ptr m_seinals{};
};

// For HJPluginCodec details, see doc/HJPluginCodec.md.

class HJPluginCodec : public HJPlugin
{
public:
	HJPluginCodec(const std::string& i_name = "HJPluginCodec", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_identify, i_graphInfo) {}
    virtual ~HJPluginCodec() { done(); }

protected:
	// HJStreamInfo::Ptr streamInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// bool createThread
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
    virtual void afterInit() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
    virtual int runFlush() override;

    virtual HJMediaType getType() = 0;
	virtual HJBaseCodec::Ptr createCodec() = 0;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo);
    virtual void releaseCodec();

	std::atomic<size_t> m_inputKeyHash{};
	HJBaseCodec::Ptr m_codec{};

protected:
    virtual int processFlushFrame(std::string& route, HJMediaFrame::Ptr& inFrame);
    virtual int processMediaFrame(std::string& route, HJMediaFrame::Ptr& inFrame);
    virtual int processEofFrame(std::string& route, HJMediaFrame::Ptr& inFrame);
    virtual std::tuple<int, HJMediaFrame::Ptr> getOutputFrame(std::string& route);

    void passThroughSetInput(const HJMediaFrame::Ptr& i_inFrame);
    void passThroughSetOutput(const HJMediaFrame::Ptr& i_outFrame);

private:
    void passThroughAdd(int64_t i_key, HJCodecPassThroughInfo::Ptr i_info);
    HJCodecPassThroughInfo::Ptr passThroughGet(int64_t i_key);
    void passThroughRemove(int64_t i_key);
    void passThroughReset();
    std::unordered_map<int64_t, HJCodecPassThroughInfo::Ptr> m_passThroughMap;
};

NS_HJ_END
