//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJAudioProcessor.h"

typedef struct AVAudioFifo AVAudioFifo;

NS_HJ_BEGIN
//***********************************************************************************//
class HJAudioFifo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAudioFifo>;
    HJAudioFifo(int channel, int sampleFmt, int sampleRate = 44100);
    ~HJAudioFifo();

    int init(int nbSamples = 1024);
    int addFrame(HJMediaFrame::Ptr&& frame);
    HJMediaFrame::Ptr getFrame(bool eof = false);
    void reset();

    size_t getSize() {
        return m_frames.size();
    }
    void setOutNBSamples(int nbSamples) {
        m_outNBSamples = nbSamples;
    }
    int getOutNBSamples() {
        return m_outNBSamples;
    }
private:
    const int64_t getLastDuration() {
        return m_lastDuration;
    }
    int makeFrame(bool eof = false);
private:
    int                 m_channel = 2;
    int                 m_sampleFmt = 8;
    int                 m_sampleRate = 44100;
    int                 m_outNBSamples = 1024;
    HJTimeBase         m_timeBase = { 1, 44100 };
    HJHND              m_fifo = NULL;
    int64_t             m_startTime = HJ_NOPTS_VALUE;      //pkt time base, nb samples
    int64_t             m_lastTime = HJ_NOPTS_VALUE;       //pkt time base, nb samples
    int64_t             m_lastDuration = 0;                 //pkt time base, nb samples
    HJMediaFrameList    m_frames;
    HJDataStati64      m_stats;
};

//***********************************************************************************//
class HJPCMFifo
{
public:
    using Ptr = std::shared_ptr<HJPCMFifo>;
    HJPCMFifo(int channel, int sampleFmt);
    ~HJPCMFifo();

    int write(void** data, int nbSamples);
    int peek(void** data, int nbSamples);
    int peek(void** data, int nbSamples, int offset);
    int read(void** data, int nbSamples);
    int drain(int nbSamples);
    void reset();
    int size();
    int space();
private:
    AVAudioFifo* m_fifo = NULL;
};

//***********************************************************************************//
/**
 * time base == {1, sample rate}
 */
class HJAFifoProcessor : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAFifoProcessor>;
    HJAFifoProcessor(int nbSamples = 1024);
    virtual ~HJAFifoProcessor();

    int init(const HJAudioInfo::Ptr& info);
    void done();
    int addFrame(const HJMediaFrame::Ptr& frame);
    HJMediaFrame::Ptr getFrame(int nbSamples = 0);
    void reset();
    //
    const size_t getSize() {
        HJ_AUTO_LOCK(m_mutex);
        return m_inAFrameBriefs.size();
    }
    void setSpeed(const float speed) {
        m_speed = speed;
    }
    const float getSpeed() const {
        return m_speed;
    }
protected:
    HJUtilTime getTime(int nbSamples);
private:
    std::recursive_mutex    m_mutex;
    HJAudioInfo::Ptr       m_info = nullptr;
    int                     m_outNBSamples = 1024;
    float                   m_speed = 1.0f;
    HJPCMFifo::Ptr         m_fifo = nullptr;
    int                     m_capacity = HJ_AREND_STOREAGE_CAPACITY;
    HJAFrameBriefList      m_inAFrameBriefs;
    bool                    m_eof = false;
    HJAudioProcessor::Ptr  m_audioProcessor = nullptr;
};

NS_HJ_END
