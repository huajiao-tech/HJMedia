#pragma once

#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

class HJPluginAudioOHRender : public HJPluginAudioRender
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioOHRender);

	HJPluginAudioOHRender(const std::string& i_name = "HJPluginAudioOHRender", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginAudioRender(i_name, i_graphInfo) { }
	virtual ~HJPluginAudioOHRender() {
		HJPluginAudioOHRender::done();
	}

	virtual OH_AudioData_Callback_Result onDataCallback(void* o_audioData, int32_t i_audioDataSize);

protected:
    virtual void internalSetMute() override;
	virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
	virtual bool releaseRender() override;
    virtual void setInfoAudioDuration(int64_t i_audioSamples) override;

private:
	OH_AudioRenderer* m_renderer{};
	OH_AudioStreamBuilder* m_streamBuilder{};
    HJMediaFrame::Ptr m_kernalFrame{};
};

NS_HJ_END
