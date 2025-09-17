#pragma once

#include "HJPlugin.h"
#include "HJBaseCodec.h"
#include "HJPrerequisites.h"

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
    HJCodecPassThroughInfo(int64_t i_demuxSysTime, bool i_bKey):
        m_demuxSysTime(i_demuxSysTime)
        ,m_bKey(i_bKey)
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
private:
    int64_t m_demuxSysTime = 0;
    bool m_bKey = false;
};

class HJPluginCodec : public HJPlugin
{
public:
	HJPluginCodec(const std::string& i_name = "HJPluginCodec", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPlugin(i_name, i_graphInfo) { }
	virtual ~HJPluginCodec() {
		HJPluginCodec::done();
	}

protected:
	// HJStreamInfo::Ptr streamInfo = nullptr
	// HJLooperThread::Ptr thread = nullptr
	// bool createThread
	// HJListener pluginListener
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;
	virtual void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;
//	virtual int flush() override;

	virtual HJBaseCodec::Ptr createCodec() = 0;
	virtual int initCodec(const HJStreamInfo::Ptr& i_streamInfo);
    virtual void releaseCodec();

	std::atomic<size_t> m_inputKeyHash{};
	HJBaseCodec::Ptr m_codec{};
	int m_frameFlag{};
    

    void passThroughSetInput(const HJMediaFrame::Ptr& i_inFrame);
    void passThroughSetOutput(const HJMediaFrame::Ptr& i_outFrame);

    void passThroughAdd(int64_t i_key, HJCodecPassThroughInfo::Ptr i_info);
    HJCodecPassThroughInfo::Ptr passThroughGet(int64_t i_key);
    void passThroughRemove(int64_t i_key);
    void passThroughReset();
    std::unordered_map<int64_t, HJCodecPassThroughInfo::Ptr> m_passThroughMap;
};

NS_HJ_END
