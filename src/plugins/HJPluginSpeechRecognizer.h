#pragma once

#include "HJPlugin.h"
#include "HJMediaFrameDeque.h"

NS_HJ_BEGIN

class HJPluginSpeechRecognizer : public HJPlugin
{
public:
    using Call = std::function<void(const HJMediaFrame::Ptr &i_frame)>;

    HJ_DEFINE_CREATE(HJPluginSpeechRecognizer);

    HJPluginSpeechRecognizer(const std::string& i_name = "HJPluginSpeechRecognizer", HJKeyStorage::Ptr i_graphInfo = nullptr)
        : HJPlugin(i_name, i_graphInfo) { }
    virtual ~HJPluginSpeechRecognizer() {
        HJPluginSpeechRecognizer::done();
    }

    virtual void onOutputUpdated() override {}

    Call m_call = nullptr;

protected:
    // HJAudioInfo::Ptr audioInfo
    // HJLooperThread::Ptr thread = nullptr
    int internalInit(HJKeyStorage::Ptr i_param) override;

    void internalRelease() override;

    void onInputAdded(size_t i_srcKeyHash, HJMediaType i_type) override;

    int runTask(int64_t* o_delay = nullptr) override;

    void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;

    std::atomic<size_t> m_inputKeyHash{};
    HJAudioInfo::Ptr m_audioInfo{};
};

NS_HJ_END
