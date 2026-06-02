#pragma once

#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <atomic>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

// See doc/HJPluginAudioOHRender.md for details.

class HJPluginAudioOHRender : public HJPluginAudioRender
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioOHRender);

	HJPluginAudioOHRender(const std::string& i_name = "HJPluginAudioOHRender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginAudioRender(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginAudioOHRender() { done(); }
//    virtual int flush(size_t i_srcKeyHash) override;
    
	virtual OH_AudioData_Callback_Result onDataCallback(void* o_audioData, int32_t i_audioDataSize);

protected:
    virtual int64_t getRenderTimestamp() override;
	virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
	virtual void releaseRender() override;

private:
	OH_AudioRenderer* m_renderer{};
	OH_AudioStreamBuilder* m_streamBuilder{};

private:
    int setStreamRunning(bool i_running, bool i_eofStop = false);
};

NS_HJ_END
