//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJARenderBase.h"
#include "HJAudioConverter.h"
#include "HJAudioFifo.h"

struct ma_context;
struct ma_device_config;
struct ma_device;

NS_HJ_BEGIN
//***********************************************************************************//
class HJARenderMini : public HJARenderBase
{
public:
    typedef std::shared_ptr<HJARenderMini> Ptr;
    HJARenderMini();
    virtual ~HJARenderMini();

    virtual int init(const HJAudioInfo::Ptr& info, ARCallback cb = nullptr) override;
    virtual int start() override;
    virtual int pause() override;
    //virtual int resume() override;
    virtual int reset() override;
    virtual int stop() override;
    virtual int write(const HJMediaFrame::Ptr rawFrame) override;

    //
    virtual void setVolume(const float volume) override;
private:
    static void outAudioCallback(ma_device* dev, void* output, const void* input, unsigned int cnt);
private:
    ma_context*             m_actx = NULL;
    ma_device_config*       m_adevCfg = NULL;
    ma_device*              m_adev = NULL;
    
    HJAudioConverter::Ptr  m_converter = nullptr;
    HJAFifoProcessor::Ptr  m_fifoProcessor = nullptr;
    HJNipMuster::Ptr       m_nipMuster = nullptr;
};

NS_HJ_END
