//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAudioFifo.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
#define HJ_LAST_SKIP_OFFSET    500     //ms
//***********************************************************************************//
HJAudioFifo::HJAudioFifo(int channel, int sampleFmt, int sampleRate)
    : m_channel(channel)
    , m_sampleFmt(sampleFmt)
    , m_sampleRate(sampleRate)
{
    m_timeBase = { 1, sampleRate };
}

HJAudioFifo::~HJAudioFifo()
{
    m_frames.clear();
    if (m_fifo) {
        AVAudioFifo* fifo = (AVAudioFifo*)m_fifo;
        av_audio_fifo_free(fifo);
        m_fifo = NULL;
    }
}

int HJAudioFifo::init(int nbSamples/* = 1024*/)
{
    int res = HJ_OK;
    m_outNBSamples = nbSamples;
    HJLogi("HJAudioFifo begin, nbSamples:" + HJ2STR(nbSamples));
    //
    AVAudioFifo* fifo = av_audio_fifo_alloc((enum AVSampleFormat)m_sampleFmt, m_channel, 1);
    if (!fifo) {
        HJLoge("create audio fifo error");
        return HJErrNewObj;
    }
    m_fifo = fifo;
    HJLogi("HJAudioFifo end");

    return res;
}

/**
 * time base == sample rate
 * 1: samples <= pts -- ok
 * 2: samples > pts -- to do (remove some PCM)
 */
int HJAudioFifo::addFrame(HJMediaFrame::Ptr&& frame)
{
    if (!frame || !m_fifo) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    AVAudioFifo* fifo = (AVAudioFifo*)m_fifo;
    do
    {
        AVFrame* avf = (AVFrame*)frame->getAVFrame();
        if (!avf) {
            break;
        }
        //HJLogi("audio fifo add AVFrame pts:" + HJ2STR(frame->m_pts) + ", dts:" + HJ2STR(frame->m_dts) + ", time base:" + HJ2STR(frame->m_timeBase.num) + "/" + HJ2STR(frame->m_timeBase.den)
        //    + ", avf pts:" + HJ2STR(avf->pts) + ", dts:" + HJ2STR(avf->best_effort_timestamp) + ", time base:" + HJ2STR(avf->time_base.num) + "/" + HJ2STR(avf->time_base.den) + ", nb_samples:" + HJ2STR(avf->nb_samples));
        if (HJ_NOPTS_VALUE == m_lastTime && (HJ_NOPTS_VALUE != avf->pts)) {
            m_startTime = m_lastTime = avf->pts;
        }
        if (HJ_NOPTS_VALUE == m_lastTime) {
            HJLoge("error, last time invalid");
            return HJErrNotAlready;
        }
        int64_t deltaPts = m_stats.add(avf->pts - (m_lastTime + m_lastDuration));
        //HJLogi("audio fifo delta pts:" + HJ2STR(deltaPts));
        if (HJ_ABS(deltaPts) >= m_sampleRate){ //avf->nb_samples) {
            HJLogw("warning, audio fifo add avframe pts:" + HJ2STR(frame->m_pts) + ", pkt pts:" + HJ2STR(avf->pts) + "skiped, warning, deltaPts:" + HJ2STR(deltaPts));
            //makeFrame(true);
            m_lastTime = avf->pts;
            m_stats.reset();
        }
        //if (HJ_NOPTS_VALUE != m_lastTime && avf->pts > (m_lastTime + m_lastDuration + av_ms_to_time(HJ_LAST_SKIP_OFFSET, avf->time_base))) 
        //{
        //    HJLogw("warning, audio fifo add avframe pts:" + HJ2STR(frame->m_pts) + ", pkt pts:" + HJ2STR(avf->pts) + "skiped, warning");
        //    makeFrame(true);
        //    m_lastTime = avf->pts;
        //}
        res = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + avf->nb_samples);
        if (res < 0) {
            HJLoge("add audio samples error");
            break;
        }
        int cnt = av_audio_fifo_write(fifo, (void**)avf->data, avf->nb_samples);
        if (cnt < avf->nb_samples) {
            HJLoge("audio fifo write error");
            res = HJErrFFAVFrame;
            break;
        }
        //
        makeFrame();
        m_lastDuration = av_audio_fifo_size(fifo); //av_time_to_ms(lastNBSamples, avf->time_base);
        //HJLogi("audio fifo add AVFrame last samples£º" + HJ2STR(m_lastDuration));
    } while (false);

    return res;
}

HJMediaFrame::Ptr HJAudioFifo::getFrame(bool eof)
{
    if (!m_fifo) {
        return nullptr;
    }
    HJMediaFrame::Ptr mavf = nullptr;
    if (eof) {
        makeFrame(eof);
    }
    if (m_frames.size() > 0) {
        mavf = m_frames.front();
        m_frames.pop_front();

        //AVFrame* avf = (AVFrame*)mavf->getAVFrame();
        //HJLogi("audio fifo get AVFrame pts:" + HJ2STR(mavf->m_pts) + ", dts:" + HJ2STR(mavf->m_dts) + ", time base:" + HJ2STR(mavf->m_timeBase.num) + "/" + HJ2STR(mavf->m_timeBase.den)
        //	+ ", avf pts:" + HJ2STR(avf->pts) + ", dts:" + HJ2STR(avf->best_effort_timestamp) + ", time base:" + HJ2STR(avf->time_base.num) + "/" + HJ2STR(avf->time_base.den));
    }
    //if (mavf) {
    //    HJLogi("202308161754 frame info" + mavf->formatInfo());
    //}

    return mavf;
}

int HJAudioFifo::makeFrame(bool eof/* = false*/)
{
    if (!m_fifo) {
        return HJErrNotAlready;
    }

    int res = HJ_OK;
    AVAudioFifo* fifo = (AVAudioFifo*)m_fifo;
    int lastNBSamples = av_audio_fifo_size(fifo);
    while (lastNBSamples >= m_outNBSamples ||
        (eof && lastNBSamples > 0))
    {
        HJMediaFrame::Ptr mavf = HJMediaFrame::makeSilenceAudioFrame(m_channel, m_sampleRate, m_sampleFmt, m_outNBSamples);
        if (!mavf) {
            HJLoge("error, make null frame");
            res = HJErrNewObj;
            break;
        }
        AVFrame* avf = (AVFrame*)mavf->getAVFrame();
        if (!avf) {
            HJLoge("error, get null avf");
            res = HJErrNewObj;
            break;
        }
        int rdNBSamples = eof ? HJ_MIN(lastNBSamples, m_outNBSamples) : m_outNBSamples;
        int cnt = av_audio_fifo_read(fifo, (void**)avf->data, rdNBSamples);
        if (cnt < rdNBSamples) {
            HJLoge("error, fifo read");
            res = HJErrFIFOREAD;
            break;
        }
        mavf->setPTSDTS(m_lastTime, m_lastTime, m_timeBase);
        mavf->setDuration(m_outNBSamples, m_timeBase);
        //HJLogi("audio fifo make AVFrame pts:" + HJ2STR(mavf->m_pts) + ", dts:" + HJ2STR(mavf->m_dts) + ", time base:" + HJ2STR(mavf->m_timeBase.num) + "/" + HJ2STR(mavf->m_timeBase.den)
        //    + ", avf pts:" + HJ2STR(avf->pts) + ", dts:" + HJ2STR(avf->best_effort_timestamp) + ", time base:" + HJ2STR(avf->time_base.num) + "/" + HJ2STR(avf->time_base.den) + ", rdNBSamples:" + HJ2STR(rdNBSamples) + ", eof:" + HJ2STR(eof));
        //
        m_frames.push_back(std::move(mavf));
        //
        m_lastTime += m_outNBSamples;   //av_time_to_ms(m_outNBSamples, avf->time_base);
        lastNBSamples = av_audio_fifo_size(fifo);
    }
    return res;
}

void HJAudioFifo::reset()
{
    AVAudioFifo* fifo = (AVAudioFifo*)m_fifo;
    if (fifo) {
        av_audio_fifo_reset(fifo);
    }
    m_startTime = HJ_NOPTS_VALUE;
    m_lastTime = HJ_NOPTS_VALUE;
    m_lastDuration = 0;
    m_frames.clear();
}

//***********************************************************************************//
HJPCMFifo::HJPCMFifo(int channel, int sampleFmt)
{
    m_fifo = av_audio_fifo_alloc((enum AVSampleFormat)sampleFmt, channel, 1);
    if (!m_fifo) {
        HJLoge("create audio fifo error");
    }
}

HJPCMFifo::~HJPCMFifo()
{
    if (m_fifo) {
        av_audio_fifo_free(m_fifo);
        m_fifo = NULL;
    }
}

int HJPCMFifo::write(void** data, int nbSamples)
{
    if (!m_fifo || !data) {
        return HJErrNotAlready;
    }
    int res = av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + nbSamples);
    if (res < 0) {
        HJLoge("error, audio fifo realloc failed");
        return res;
    }
    return av_audio_fifo_write(m_fifo, data, nbSamples);
}
int HJPCMFifo::peek(void** data, int nbSamples)
{
    if (!m_fifo || !data) {
        return HJErrNotAlready;
    }
    return av_audio_fifo_peek(m_fifo, data, nbSamples);
}
int HJPCMFifo::peek(void** data, int nbSamples, int offset)
{
    if (!m_fifo || !data) {
        return HJErrNotAlready;
    }
    return av_audio_fifo_peek_at(m_fifo, data, nbSamples, offset);
}
int HJPCMFifo::read(void** data, int nbSamples)
{
    if (!m_fifo || !data) {
        return HJErrNotAlready;
    }
    return av_audio_fifo_read(m_fifo, data, nbSamples);
}
int HJPCMFifo::drain(int nbSamples)
{
    if (!m_fifo) {
        return HJErrNotAlready;
    }
    return av_audio_fifo_drain(m_fifo, nbSamples);
}
void HJPCMFifo::reset()
{
    if (!m_fifo) {
        return;
    }
    av_audio_fifo_reset(m_fifo);
}
int HJPCMFifo::size()
{
    if (!m_fifo) {
        return 0;
    }
    return av_audio_fifo_size(m_fifo);
}
int HJPCMFifo::space()
{
    if (!m_fifo) {
        return 0;
    }
    return av_audio_fifo_space(m_fifo);
}

//***********************************************************************************//
HJAFifoProcessor::HJAFifoProcessor(int nbSamples)
    : m_outNBSamples(nbSamples)
{

}

HJAFifoProcessor::~HJAFifoProcessor()
{
    done();
}

int HJAFifoProcessor::init(const HJAudioInfo::Ptr& info)
{
    int res = HJ_OK;
    do
    {
        m_info = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
        m_fifo = std::make_shared<HJPCMFifo>(m_info->m_sampleFmt, m_info->m_channels);
        if (!m_fifo) {
            HJLoge("error, create audio fifo failed");
            res = HJErrNewObj;
            break;
        }
    } while (false);

    return res;
}

void HJAFifoProcessor::done()
{
    reset();
    m_fifo = nullptr;
}

int HJAFifoProcessor::addFrame(const HJMediaFrame::Ptr& frame)
{
    if (!frame || !m_fifo) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    HJ_AUTO_LOCK(m_mutex);
    do
    {
        if (HJFRAME_EOF == frame->getFrameType()) {
            m_eof = true;
            break;
        }
        const auto ainfo = frame->getAudioInfo();
        if (*ainfo != *m_info) {
            HJLoge("error, input audio frame format unsupport");
            break;
        }
        HJMediaFrame::Ptr mavf = frame;
        //
        if (!m_audioProcessor && (1.0f != m_speed)) {
            m_audioProcessor = HJAudioProcessor::create();
            //m_audioProcessor = std::make_shared<HJAudioProcessor>();
            m_audioProcessor->setSpeed(m_speed);
        }
        if (m_audioProcessor)
        {
            if (m_audioProcessor->getSpeed() != m_speed) {
                m_audioProcessor->setSpeed(m_speed);
            }
            
            res = m_audioProcessor->addFrame(mavf);
            if (HJ_OK != res) {
                HJLoge("error, audio processor add frame failed");
                break;
            }
            auto outFrame = m_audioProcessor->getFrame();
            if (!outFrame) {
                HJLoge("error, audio processor get frame failed");
                break;
            }
            mavf = std::move(outFrame);

            //HJMediaFrame::Ptr outFrame = nullptr;
            //res = m_audioProcessor->getFrame(outFrame);
            //if (HJ_OK != res) {
            //    HJLoge("error, audio processor get frame failed");
            //    break;
            //}
            //mavf = std::move(outFrame);
        }
        if (!mavf) {
            HJLogw("warning, a frame is null");
            break;
        }
        //
        AVFrame* avf = (AVFrame*)mavf->getAVFrame();
        if (!avf) {
            res = HJErrInvalid;
            HJLoge("error, get avframe failed");
            break;
        }
        int cnt = m_fifo->write((void**)avf->data, avf->nb_samples);
        if (cnt < avf->nb_samples) {
            HJFLoge("error, audio fifo write failed, cnt:{}", cnt);
            res = HJErrFFAVFrame;
            break;
        }
        HJAFrameBrief brief({ avf->pts, { avf->time_base.num, avf->time_base.den } }, avf->nb_samples);
        brief.m_speed = mavf->getSpeed();
        m_inAFrameBriefs.emplace_back(std::move(brief));
    } while (false);

    return res;
}

HJMediaFrame::Ptr HJAFifoProcessor::getFrame(int nbSamples)
{
    if (!m_fifo) {
        return nullptr;
    }
    int res = HJ_OK;
    HJMediaFrame::Ptr frame = nullptr;
    HJ_AUTO_LOCK(m_mutex);
    do
    {
        int outNBSamples = (nbSamples > 0) ? nbSamples : m_outNBSamples;
        int lastNBSamples = m_fifo->size();
        if (lastNBSamples >= outNBSamples || (m_eof && lastNBSamples > 0))
        {
            HJMediaFrame::Ptr mavf = HJMediaFrame::makeSilenceAudioFrame(m_info->m_channels, m_info->m_samplesRate , m_info->m_sampleFmt, outNBSamples);
            if (!mavf) {
                HJLoge("error, make null frame");
                res = HJErrNewObj;
                break;
            }
            AVFrame* avf = (AVFrame*)mavf->getAVFrame();
            if (!avf) {
                HJLoge("error, get null avf");
                res = HJErrNewObj;
                break;
            }
            int rdNBSamples = m_eof ? HJ_MIN(lastNBSamples, outNBSamples) : outNBSamples;
            int cnt = m_fifo->read((void**)avf->data, rdNBSamples);
            if (cnt < rdNBSamples) {
                HJLoge("error, fifo read");
                res = HJErrFIFOREAD;
                break;
            }
            avf->nb_samples = cnt;

            // pts
            HJUtilTime pts = getTime(cnt);
            mavf->setPTSDTS(pts.getValue(), pts.getValue(), pts.getTimeBase());
            mavf->setDuration(cnt, pts.getTimeBase());
            mavf->setMTS({ pts, pts });
            //
            frame = std::move(mavf);
        }
    } while (false);

    return frame;
}

void HJAFifoProcessor::reset()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_audioProcessor) {
        m_audioProcessor->reset();
    }
    if (m_fifo) {
        m_fifo->reset();
    }
    m_inAFrameBriefs.clear();
    m_eof = false;
    return;
}

HJUtilTime HJAFifoProcessor::getTime(int nbSamples)
{
    HJUtilTime pts = HJUtilTime::HJTimeNOTS;
    int outSamples = 0;
    for (auto it = m_inAFrameBriefs.begin(); it != m_inAFrameBriefs.end();)
    {
        if (HJ_NOTS_VALUE == pts.getValue()) {
            pts = it->m_pts;
        }
        outSamples += HJ_MIN(it->m_nbSamples, it->m_lastNBSamples);
        if (outSamples < nbSamples) {
            it = m_inAFrameBriefs.erase(it);
        } else if (outSamples == nbSamples) {
            it = m_inAFrameBriefs.erase(it);
            break;
        } else {
            it->m_lastNBSamples = outSamples - nbSamples;
            it->m_pts.addOffset((it->m_nbSamples - it->m_lastNBSamples) * it->m_speed);
            break;
        }
    }
    return pts;
}

NS_HJ_END
