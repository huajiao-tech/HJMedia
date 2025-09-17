#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

class HJPluginAudioWORender : public HJPluginAudioRender
{
public:
	HJ_DEFINE_CREATE(HJPluginAudioWORender);

	HJPluginAudioWORender(const std::string& i_name = "HJPluginAudioWORender", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginAudioRender(i_name, i_graphInfo) { }
	virtual ~HJPluginAudioWORender() {
		HJPluginAudioWORender::done();
	}
	virtual void onOutputUpdated() override;

protected:
	virtual int runTask(int64_t* o_delay = nullptr) override;
	virtual void internalSetMute() override;
	virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
	virtual bool releaseRender() override;

private:
	HWAVEOUT m_render{};
	HJMediaFrameDeque m_kernalFrames;
};

NS_HJ_END
