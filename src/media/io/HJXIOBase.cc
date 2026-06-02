//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOBase.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJXIOBase::HJXIOBase()
{
}

HJXIOBase::~HJXIOBase()
{
}

//***********************************************************************************//
HJXIOInterrupt::HJXIOInterrupt()
{
    m_interruptCB = (AVIOInterruptCB *)av_mallocz(sizeof(AVIOInterruptCB));
    if (!m_interruptCB) {
        return;
    }
    m_interruptCB->callback = onInterruptCB;
    m_interruptCB->netCallback = onInterruptNetCB;
    m_interruptCB->opaque = this;
}

HJXIOInterrupt::~HJXIOInterrupt()
{
    if(m_interruptCB) {
        av_freep(&m_interruptCB);
    }
}

void HJXIOInterrupt::setInterruptCB(AVIOInterruptCB* cb)
{
    if (!cb) {
        return;
    }
    m_interruptCB->callback = cb->callback;
    m_interruptCB->netCallback = cb->netCallback;

    return;
}

int HJXIOInterrupt::onInterruptNetNotify(const HJNotification::Ptr ntfy)
{
    if (m_listener) {
        return m_listener(ntfy);
    }
    return HJ_OK;
}

int HJXIOInterrupt::onInterruptCB(void *ctx)
{
    HJXIOInterrupt* xio = (HJXIOInterrupt *)ctx;
    if (xio) {
        if (xio->m_isQuit) {
            return true;
        }
        return xio->isIOTimeout();
    }
    return false;
}

int HJXIOInterrupt::onInterruptNetCB(void* ctx, int state)
{
    HJXIOInterrupt* xio = (HJXIOInterrupt *)ctx;
    if (xio) {
        return xio->onInterruptNetNotify(HJMakeNotification(HJ_NOTIFY_IO_INTERRUPT_NET, state));
    }
    return HJ_OK;
}

bool HJXIOInterrupt::isIOTimeout()
{
    if (m_ioTimeout != HJ_NOPTS_VALUE && m_ioRunTime != HJ_NOPTS_VALUE) {
        auto delta = HJCurrentSteadyUS() - m_ioRunTime;
        //HJFLogi("HJXIOInterrupt::isIOTimeout() delta:{} timeout:{}", delta, m_ioTimeout);
        if (delta > m_ioTimeout) {
            HJFLogw("warning, HJXIOInterrupt::isIOTimeout() delta:{} timeout:{}", delta, m_ioTimeout);
            return true;
        }
    }
    return false;
}

NS_HJ_END
