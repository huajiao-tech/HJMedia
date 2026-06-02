#pragma once

#include <atomic>

#include "HJPlugin.h"
#include "HJMediaFrameDeque.h"

NS_HJ_BEGIN

class HJPluginAudioInput : public HJPlugin
{
public:
    HJ_DEFINE_CREATE(HJPluginAudioInput);

    HJPluginAudioInput(const std::string& i_name = "HJPluginAudioInput", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
        : HJPlugin(i_name, i_identify, i_graphInfo)
        , m_pending_frames("HJPluginAudioInputDeque", i_identify) {}
    virtual ~HJPluginAudioInput() { done(); }

    int bindInput(const std::string& i_input_id, const HJAudioInfo::Ptr& i_input_info);
    int unbindInput();
    int pushFrame(const HJMediaFrame::Ptr& i_frame);
    int flushInput();
    int signalEof();

    const std::string& getInputId() const {
        return m_input_id;
    }
    const HJAudioInfo::Ptr& getInputInfo() const {
        return m_input_info;
    }
    bool hasBinding() const {
        return !m_input_id.empty() && (m_input_info != nullptr);
    }

protected:
    int internalInit(HJKeyStorage::Ptr i_param) override;
    void internalRelease() override;
    int runTask(int64_t* o_delay = nullptr) override;
    int runFlush() override;

private:
    int validateFrame(const HJMediaFrame::Ptr& i_frame) const;

    std::string m_input_id{};
    HJAudioInfo::Ptr m_input_info{};
    HJMediaFrameDeque m_pending_frames;
    std::atomic<bool> m_eof{ false };
};

NS_HJ_END
