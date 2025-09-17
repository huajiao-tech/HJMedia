#include "HJFLog.h"
#include "HJPacketChecker.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJPacketChecker::HJPacketChecker()
{
    m_vtracker = std::make_shared<HJBitrateTracker>(2 * HJ_MS_PER_SEC);
    m_atracker = std::make_shared<HJBitrateTracker>(2 * HJ_MS_PER_SEC);
}

HJPacketChecker::~HJPacketChecker()
{

}

int HJPacketChecker::checkVideoPacket(AVStream* stream, AVPacket* pkt, int64_t pts, int64_t dts)
{
	if (!stream || !pkt) {
		return HJErrInvalidParams;
	}
	if ((pkt->flags & AV_PKT_FLAG_KEY))
    {
        //HJFLogi("entry, pkt pts:{}, dts:{}, size:{}", pts, dts, pkt->size);
        AVCodecParameters* codecparams = stream->codecpar;
        //
        int res = HJ_OK;
        do
        {
            if (!m_extraVideoBuffer || m_extraVideoBuffer->getID() != codecparams->codec_id)
            {
                if (m_extraVideoBuffer) {
                    m_streamID++;
                    resetTS();
                }
                m_extraVideoBuffer = std::make_shared<HJBuffer>(codecparams->extradata, codecparams->extradata_size);
                if (!m_extraVideoBuffer) {
                    res = HJErrMemAlloc;
                    break;
                }
                m_extraVideoBuffer->setID(codecparams->codec_id);
                
                HJFLogi("{}, check packet video, first extra data, pts:{}, dts:{}, m_streamID:{}", getName(), pts, dts, m_streamID);
            }
            else
            {
                auto newExtraBuffer = hj_get_extradata_buffer(pkt);
                if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraVideoBuffer->data(), m_extraVideoBuffer->size(), newExtraBuffer->data(), newExtraBuffer->size())) {
                    m_extraVideoBuffer = newExtraBuffer;
                    m_streamID++;
                    resetTS();
                    HJFLogi("{}, check packet video, new extra data,  pts:{}, dts:{}, m_streamID:{}", getName(), pts, dts, m_streamID);
                }
            }
        } while (false);
    }
    m_lastVideoPTS = pts;
	m_lastVideoDTS = dts;
    //
    if(pkt->size > 0) {
        int64_t vkbps = (int64_t)(m_vtracker->addData(pkt->size) / 1000);
        m_vstats = {vkbps, m_vtracker->getFPS()};
//        HJFLogi("video packet size:{}, pts:{}", pkt->size, pts);
    }

	return 0;
}

int HJPacketChecker::checkAudioPacket(AVStream* stream, AVPacket* pkt, int64_t pts, int64_t dts)
{
	if (!stream || !pkt) {
		return HJErrInvalidParams;
	}
	//HJFLogi("entry, pkt pts:{}, dts:{}, size:{}", pts, dts, pkt->size);
	AVCodecParameters* codecparams = stream->codecpar;
	//
	int res = HJ_OK;
	do {

		if (!m_extraAudioBuffer || m_extraAudioBuffer->getID() != codecparams->codec_id)
		{
			if (m_extraAudioBuffer) {
				m_streamID++;
                resetTS();
			}
			m_extraAudioBuffer = std::make_shared<HJBuffer>(codecparams->extradata, codecparams->extradata_size);
			if (!m_extraAudioBuffer) {
				res = HJErrMemAlloc;
				break;
			}
			m_extraAudioBuffer->setID(codecparams->codec_id);

			HJFLogi("{}, check packet audio, first extra data, pts:{}, dts:{}, m_streamID:{}", getName(), pts, dts, m_streamID);
		}
		else
		{
            auto newExtraBuffer = hj_get_extradata_buffer(pkt);
            if (newExtraBuffer && !HJUtilitys::memoryCompare(m_extraAudioBuffer->data(), m_extraAudioBuffer->size(), newExtraBuffer->data(), newExtraBuffer->size())) {
                m_extraAudioBuffer = newExtraBuffer;
                m_streamID++;
                resetTS();
                HJFLogi("{}, check packet audio, new extra data,  pts:{}, dts:{}, m_streamID:{}", getName(), pts, dts, m_streamID);
                break;
            }
            if (HJ_NOTS_VALUE != m_lastAudioDTS)
            {
                int64_t delta = dts - m_lastAudioDTS;
                if (dts < m_lastAudioDTS || delta > m_gapThreshold)
                {
                    m_streamID++;
                    resetTS();
                    HJFLogi("{}, check packet audio, dts:{}, delta:{}, m_lastAudioDTS:{}, m_streamID:{}", getName(), dts, delta, m_lastAudioDTS, m_streamID);
                    break;
                }
            }
		}
	} while (false);
    m_lastAudioPTS = pts;
	m_lastAudioDTS = dts;
    //
    if(pkt->size > 0) {
        int64_t akbps = (int64_t)(m_atracker->addData(pkt->size) / 1000);
        m_astats = {akbps, m_atracker->getFPS()};
//        HJFLogi("audio packet size:{}, pts:{}", pkt->size, pts);
    }

	return 0;
}

void HJPacketChecker::reset()
{
	m_extraAudioBuffer = nullptr;
    m_extraVideoBuffer = nullptr;
	m_streamID = 0;
    resetTS();
}

void HJPacketChecker::resetTS()
{
    m_lastVideoPTS = HJ_NOTS_VALUE;
    m_lastAudioPTS = HJ_NOTS_VALUE;
    m_lastVideoDTS = HJ_NOTS_VALUE;
    m_lastAudioDTS = HJ_NOTS_VALUE;
}

int64_t HJPacketChecker::getSyncDelta()
{
    if(AV_NOPTS_VALUE != m_lastVideoPTS && AV_NOPTS_VALUE != m_lastAudioPTS) {
        return m_lastVideoPTS - m_lastAudioPTS;
    }
    return AV_NOPTS_VALUE;
}

NS_HJ_END
