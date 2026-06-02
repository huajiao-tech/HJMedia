#include "HJPluginAudioInput.h"

#include "HJFLog.h"

NS_HJ_BEGIN

int HJPluginAudioInput::internalInit(HJKeyStorage::Ptr i_param)
{
    auto param = HJKeyStorage::dupFrom(i_param ? i_param : std::make_shared<HJKeyStorage>());
    HJLooperThread::Ptr thread{};
    if (i_param != nullptr && i_param->haveValue("thread")) {
        thread = i_param->getValue<HJLooperThread::Ptr>("thread");
    }
    (*param)["createThread"] = (thread == nullptr);
    return HJPlugin::internalInit(param);
}

void HJPluginAudioInput::internalRelease()
{
    m_input_id.clear();
    m_input_info = nullptr;
    m_eof.store(false);
    m_pending_frames.flush();
    HJPlugin::internalRelease();
}

int HJPluginAudioInput::bindInput(const std::string& i_input_id, const HJAudioInfo::Ptr& i_input_info)
{
    if (i_input_id.empty() || i_input_info == nullptr) {
        return HJErrInvalidParams;
    }

    return SYNC_PROD_LOCK([this, &i_input_id, &i_input_info] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        m_input_id = i_input_id;
        m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(i_input_info->dup());
        m_eof.store(false);
        return HJ_OK;
    });
}

int HJPluginAudioInput::unbindInput()
{
    return SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        m_pending_frames.flush();
        m_input_id.clear();
        m_input_info = nullptr;
        m_eof.store(false);
        return HJ_OK;
    });
}

int HJPluginAudioInput::validateFrame(const HJMediaFrame::Ptr& i_frame) const
{
    if (i_frame == nullptr || !hasBinding()) {
        return HJErrInvalidParams;
    }
    if (i_frame->getType() != HJMEDIA_TYPE_AUDIO) {
        return HJErrInvalidData;
    }
    return HJ_OK;
}

int HJPluginAudioInput::pushFrame(const HJMediaFrame::Ptr& i_frame)
{
    int ret = validateFrame(i_frame);
    if (ret != HJ_OK) {
        return ret;
    }

    auto frame = i_frame;
    if (frame->getAudioInfo() == nullptr) {
        frame->setInfo(std::dynamic_pointer_cast<HJAudioInfo>(m_input_info->dup()));
    }
    HJFPERLogi("{}, frame info:{}", getName(), i_frame->formatInfo());

    if (!m_pending_frames.store(frame)) {
        return HJErrFatal;
    }
    postTask();
    return HJ_OK;
}

int HJPluginAudioInput::flushInput()
{
    m_eof.store(false);
    m_pending_frames.flush(true);
    postTask();
    return HJ_OK;
}

int HJPluginAudioInput::signalEof()
{
    if (!hasBinding()) {
        return HJErrInvalidParams;
    }

    m_eof.store(true);
    auto eof_frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    if (!m_pending_frames.store(eof_frame)) {
        return HJErrFatal;
    }
    postTask();
    return HJ_OK;
}

int HJPluginAudioInput::runFlush()
{
    m_pending_frames.flush();
    return HJPlugin::runFlush();
}

int HJPluginAudioInput::runTask(int64_t* o_delay)
{
    (void)o_delay;
    int ret = HJ_OK;
    do {
        auto status = SYNC_CONS_LOCK([this] {
            return m_status;
        });
        if (status == HJSTATUS_Done) {
            ret = HJErrAlreadyDone;
            break;
        }
        if (status < HJSTATUS_Inited) {
            ret = HJ_WOULD_BLOCK;
            break;
        }

        HJMediaFrame::Ptr out_frame = m_pending_frames.receive();
        if (out_frame == nullptr) {
            ret = HJ_WOULD_BLOCK;
            break;
        }

        if (out_frame->isClearFrame()) {
            ret = runFlush();
            break;
        }
        HJFPERLogi("{}, deliver frame info:{}, size:{}", getName(), out_frame->formatInfo(), m_pending_frames.size());

        deliverToOutputs(out_frame);
        if (out_frame->isEofFrame()) {
            report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_EOF, getID());
        }
    } while (false);
    return ret;
}

NS_HJ_END
