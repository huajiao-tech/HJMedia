//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAudioProcessor.h"
#include "sonic.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJAudioProcessor::HJAudioProcessor()
{
}
HJAudioProcessor::~HJAudioProcessor()
{

}

int HJAudioProcessor::init(const HJAudioInfo::Ptr& info)
{
	int res = HJ_OK;
	do
	{
		m_info = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
		m_stream = sonicCreateStream(m_info->m_samplesRate, m_info->m_channels);
		if (!m_stream) {
			HJLoge("error,sonic create stream failed");
			break;
		}
		sonicSetSpeed(m_stream, m_speed);
		sonicSetPitch(m_stream, m_pitch);
		sonicSetRate(m_stream, m_rate);
		sonicSetVolume(m_stream, m_volume);
		//sonicSetChordPitch(m_stream, emulateChordPitch);
		sonicSetQuality(m_stream, m_quality);
	} while (false);
	HJFLogi("end, speed:{}, pitch:{}, rate:{}, soft volume:{}, quality:{}", m_speed, m_pitch, m_rate, m_volume, m_quality);

	return res;
}

void HJAudioProcessor::done()
{
	if (m_stream) {
		sonicDestroyStream(m_stream);
		m_stream = NULL;
	}
	HJFLogi("end");
}

int HJAudioProcessor::addFrame(const HJMediaFrame::Ptr& frame)
{
	if (!frame) {
		return HJErrInvalidParams;
	}
	//HJLogi("entry");
	int res = HJ_OK;
	do
	{
		auto& ainfo = frame->getAudioInfo();
		if (!m_stream || (m_info->m_samplesRate != ainfo->m_samplesRate) || (m_info->m_channels != ainfo->m_channels)) {
			done();
			res = init(ainfo);
			if (HJ_OK != res) {
				break;
			}
		}
		if (HJFRAME_EOF == frame->getFrameType()) {
			sonicFlushStream(m_stream);
			m_eof = true;
		} else 
		{
			AVFrame* avf = (AVFrame*)frame->getAVFrame();
			if (!avf) {
				res = HJErrFFAVFrame;
				break;
			}
			checkParams();
			//sonicReadShortFromStream
			int ret = 0;
			if (AV_SAMPLE_FMT_S16 == m_info->m_sampleFmt) {
				ret = sonicWriteShortToStream(m_stream, (const short*)avf->data[0], avf->nb_samples);
			} else {
				ret = sonicWriteFloatToStream(m_stream, (const float*)avf->data, avf->nb_samples);
			}
			if (!ret) {
				HJLoge("error, sonic write stream failed");
				break;
			}
			//
			m_inTime = { avf->pts, { avf->time_base.num, avf->time_base.den } };
			if (HJ_NOTS_VALUE == m_outTime.getValue() && HJ_NOTS_VALUE != avf->pts) {
				m_outTime = { avf->pts, { avf->time_base.num, avf->time_base.den } };
			}
			if (HJ_NOPTS_VALUE == m_outTime.getValue()) {
				HJLoge("error, last time invalid");
				return HJErrNotAlready;
			}
			auto delta = m_inTime.getValue() - m_outTime.getValue();
			if (HJ_ABS(delta) > m_info->m_samplesRate) {
				HJFLogw("warning, pts skip, delta:{}", delta);
				m_outTime = { avf->pts, { avf->time_base.num, avf->time_base.den } };
			}
			//HJFLogi("sonic write samples:{}, total samples input:{}", avf->nb_samples, m_inTime.getValue());
		}
	} while (false);
	//HJLogi("end");

	return res;
}

int HJAudioProcessor::getFrame(HJMediaFrame::Ptr& frame)
{
	int res = HJ_OK;
	do
	{
		int availableSamples = sonicSamplesAvailable(m_stream);
		if (availableSamples > 0)
		{
			HJMediaFrame::Ptr mavf = HJMediaFrame::makeSilenceAudioFrame(m_info->m_channels, m_info->m_samplesRate, m_info->m_sampleFmt, availableSamples);
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
			int retSamples = 0;
			if (AV_SAMPLE_FMT_S16 == m_info->m_sampleFmt) {
				retSamples = sonicReadShortFromStream(m_stream, (short *)avf->data[0], availableSamples);
			} else {
				retSamples = sonicReadFloatFromStream(m_stream, (float*)avf->data[0], availableSamples);
			}
			if (retSamples <= 0) {
				HJLoge("error, read sonic stream failed");
				break;
			}
			avf->nb_samples = retSamples;
			//
			auto& pts = m_outTime;
			mavf->setPTSDTS(pts.getValue(), pts.getValue(), pts.getTimeBase());
			mavf->setDuration(retSamples, pts.getTimeBase());
			mavf->setSpeed(m_speed);
			mavf->setMTS({ pts, pts });
			m_outTime.addOffset(retSamples * m_speed);
			//
			frame = std::move(mavf);
			//HJFLogi("sonic read samples:{}, total samples intpu:{} output:{} speed:{}", avf->nb_samples, m_inTime.getValue(), m_outTime.getValue(), m_speed);
		}
		else if (m_eof)
		{
			frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
		}
		else {
			HJFLogw("warning, sonic available samples:{}", availableSamples);
		}
	} while (false);

	return res;
}

void HJAudioProcessor::reset()
{
	m_eof = false;
	m_inTime = HJUtilTime::HJTimeNOTS;
	m_outTime = HJUtilTime::HJTimeNOTS;
	if (m_stream) {
		sonicDestroyStream(m_stream);
		m_stream = NULL;
	}
}

void HJAudioProcessor::checkParams()
{
	if (m_speed != sonicGetSpeed(m_stream)) {
		sonicSetSpeed(m_stream, m_speed);
	}
	if (m_pitch != sonicGetPitch(m_stream)) {
		sonicSetPitch(m_stream, m_pitch);
	}
	if (m_rate != sonicGetRate(m_stream)) {
		sonicSetRate(m_stream, m_rate);
	}
	if (m_volume != sonicGetVolume(m_stream)) {
		sonicSetVolume(m_stream, m_volume);
	}
	if (m_quality != sonicGetQuality(m_stream)) {
		sonicSetQuality(m_stream, m_quality);
	}
}

NS_HJ_END


