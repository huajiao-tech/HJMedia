//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOBase.h"
#include "HJFFUtils.h"

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
        return xio->m_isQuit;
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

NS_HJ_END
