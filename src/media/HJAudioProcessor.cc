//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAudioProcessor.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

#if defined(HJ_HAVE_SONIC)
#include "sonic.h"
#endif
#if defined(HJ_HAVE_SOUNDTOUCH)
#include "SoundTouch.h"
#include "BPMDetect.h"
#endif

NS_HJ_BEGIN
//***********************************************************************************//
int HJAudioProcessor::init(const HJAudioInfo::Ptr& info) 
{
	m_info = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
	return HJ_OK;
}

int HJAudioProcessor::updateTime(AVFrame* avf)
{
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
	return HJ_OK;
}

void HJAudioProcessor::shortToFloat(const short* input, float* output, int channels, int samples)
{
	for (int i = 0; i < channels * samples; i++) {
		output[i] = input[i] / 32767.0f;
	}
}

void HJAudioProcessor::floatToShort(const float* input, short* output, int channels, int samples)
{
	for (int i = 0; i < channels * samples; i++) {
		float sample = input[i];
		sample = HJ_CLIP(sample, -1.0f, 1.0f);
		output[i] = static_cast<short>(sample * 32767.0f);
	}
}

HJAudioProcessor::Ptr HJAudioProcessor::create(HJAudioProcessorType type)
{
	HJAudioProcessor::Ptr processor = nullptr;
	switch (type)
    {
    case HJAudioProcessorSonic:
        processor = std::make_shared<HJSonicAudioProcessor>();
        break;
    case HJAudioProcessorSoundTouch:
        processor = std::make_shared<HJSTAudioProcessor>();
        break;
    default:
        break;
    }
	return processor;
}

//***********************************************************************************//
HJSonicAudioProcessor::HJSonicAudioProcessor()
{
	m_type = HJAudioProcessorSonic;
}

HJSonicAudioProcessor::~HJSonicAudioProcessor()
{
	done();
}

int HJSonicAudioProcessor::init(const HJAudioInfo::Ptr& info)
{
	int res = HJAudioProcessor::init(info);
	if (HJ_OK != res) {
		return res;
	}
	HJFLogi("entry, speed:{}, pitch:{}, rate:{}, soft volume:{}, quality:{}", m_speed, m_pitch, m_rate, m_volume, m_quality);
	m_stream = sonicCreateStream(m_info->m_samplesRate, m_info->m_channels);
	if (!m_stream) {
		HJLoge("error,sonic create stream failed");
		return res;
	}
	updateParams();
	HJFLogi("end");

	return HJ_OK;
}

void HJSonicAudioProcessor::done()
{
	HJFLogi("entry");
	if (m_stream) {
		sonicDestroyStream(m_stream);
		m_stream = NULL;
	}
	HJFLogi("end");
}

int HJSonicAudioProcessor::addFrame(const HJMediaFrame::Ptr& frame)
{
	if (!frame) {
		return HJErrInvalidParams;
	}
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
		}
		else
		{
			AVFrame* avf = (AVFrame*)frame->getAVFrame();
			if (!avf) {
				res = HJErrFFAVFrame;
				break;
			}
			updateParams();
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
			res = updateTime(avf);
			if (res != HJ_OK) {
				break;
			}
			//HJFLogi("sonic write samples:{}, total samples input:{}", avf->nb_samples, m_inTime.getValue());
		}
	} while (false);
	//HJLogi("end");

	return res;
}

HJMediaFrame::Ptr HJSonicAudioProcessor::getFrame()
{
	if (!m_stream) {
		return nullptr;
	}
	int res = HJ_OK;
	HJMediaFrame::Ptr mavf = nullptr;
	do
	{
		int availableSamples = sonicSamplesAvailable(m_stream);
		if (availableSamples > 0)
		{
			mavf = HJMediaFrame::makeSilenceAudioFrame(m_info->m_channels, m_info->m_samplesRate, m_info->m_sampleFmt, availableSamples);
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
				retSamples = sonicReadShortFromStream(m_stream, (short*)avf->data[0], availableSamples);
			}
			else {
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
			//HJFLogi("sonic read samples:{}, total samples intpu:{} output:{} speed:{}", avf->nb_samples, m_inTime.getValue(), m_outTime.getValue(), m_speed, mavf->getDuration());
		}
		else if (m_eof) {
			mavf = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
		}
		//else {
		//	HJFLogw("warning, sonic available samples:{}", availableSamples);
		//}
	} while (false);

	return mavf;
}

void HJSonicAudioProcessor::reset()
{
	if (!m_stream) {
		return;
	}
	m_eof = false;
	m_inTime = HJUtilTime::HJTimeNOTS;
	m_outTime = HJUtilTime::HJTimeNOTS;
	//
	sonicFlushStream(m_stream);
	getFrame();

	return;
}

void HJSonicAudioProcessor::updateParams()
{
	sonicSetSpeed(m_stream, m_speed);
	sonicSetPitch(m_stream, m_pitch);
	sonicSetRate(m_stream, m_rate);
	sonicSetVolume(m_stream, m_volume);
	//sonicSetChordPitch(m_stream, emulateChordPitch);
	sonicSetQuality(m_stream, m_quality);
}

void HJSonicAudioProcessor::setSpeed(const float speed)
{
	if (!m_stream || speed == m_speed) {
		return;
	}
	HJAudioProcessor::setSpeed(speed);

	if (m_speed != sonicGetSpeed(m_stream)) {
		sonicSetSpeed(m_stream, m_speed);
	}

	return;
}

//***********************************************************************************//
HJSTAudioProcessor::HJSTAudioProcessor()
{
    m_type = HJAudioProcessorSoundTouch;
}

HJSTAudioProcessor::~HJSTAudioProcessor()
{
	done();
}

int HJSTAudioProcessor::init(const HJAudioInfo::Ptr& info)
{
	int res = HJAudioProcessor::init(info);
	if (HJ_OK != res) {
		return res;
	}
	HJFLogi("entry, samplerate:{}, channels:{}", m_info->m_samplesRate, m_info->m_channels);
	m_soundtouch = std::make_shared<soundtouch::SoundTouch>();
	if (!m_soundtouch) {
		HJLoge("error, new soundtouch failed");
		return HJErrNewObj;
	}
	m_soundtouch->setSampleRate(m_info->m_samplesRate);
	m_soundtouch->setChannels(m_info->m_channels);

	//m_soundtouch->setTempoChange(0);
	m_soundtouch->setPitchSemiTones(0);
	m_soundtouch->setRateChange(0);
	//m_soundtouch->setSetting(SETTING_USE_QUICKSEEK,1);
	m_soundtouch->setSetting(SETTING_USE_AA_FILTER, 1);

	if (m_speech) {
		m_soundtouch->setSetting(SETTING_SEQUENCE_MS, 40);
		m_soundtouch->setSetting(SETTING_SEEKWINDOW_MS, 15);
		m_soundtouch->setSetting(SETTING_OVERLAP_MS, 8);
	}
	m_soundtouch->setTempoChange((m_speed - 1) * 100.0);
	HJFLogi("end");

	return HJ_OK;
}
void HJSTAudioProcessor::done()
{
	m_soundtouch = nullptr;
}

int HJSTAudioProcessor::addFrame(const HJMediaFrame::Ptr& frame)
{
	if (!frame) {
		return HJErrInvalidParams;
	}
	int res = HJ_OK;
	do
	{
		auto& ainfo = frame->getAudioInfo();
		res = checkInfo(ainfo);
		if (HJ_OK != res) {
			break;
		}

		if (HJFRAME_EOF == frame->getFrameType()) {
			m_eof = true;
		}
		else
		{
			AVFrame* avf = (AVFrame*)frame->getAVFrame();
			if (!avf) {
				res = HJErrFFAVFrame;
				break;
			}
			float* fdata = NULL;
			if (AV_SAMPLE_FMT_S16 == m_info->m_sampleFmt) {
				m_inputBufer.resize(m_info->m_channels * avf->nb_samples);
				fdata = m_inputBufer.data();	
			}
			else if (AV_SAMPLE_FMT_FLT == m_info->m_sampleFmt) {
				fdata = (float*)avf->data[0];
			}
			if (!fdata) {
				HJLoge("error, unsupport sample format:{}", m_info->m_sampleFmt);
				break;
			}
			m_soundtouch->putSamples((const soundtouch::SAMPLETYPE*)fdata, avf->nb_samples);

			//
			res = updateTime(avf);
			if (res != HJ_OK) {
				break;
			}
			//HJFLogi("soundtouch write samples:{}, total samples input:{}", avf->nb_samples, m_inTime.getValue());
		}
	} while (false);
	//HJLogi("end");

	return res;
}

HJMediaFrame::Ptr HJSTAudioProcessor::getFrame()
{
	if (!m_soundtouch) {
		return nullptr;
	}
	int res = HJ_OK;
	HJMediaFrame::Ptr mavf = nullptr;
	do
	{
		int availableSamples = 0;
		auto buffer = m_soundtouch->receiveSamples(availableSamples);
		if (buffer)
		{
			mavf = HJMediaFrame::makeSilenceAudioFrame(m_info->m_channels, m_info->m_samplesRate, m_info->m_sampleFmt, availableSamples);
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
			avf->nb_samples = availableSamples;
			//
			auto& pts = m_outTime;
			mavf->setPTSDTS(pts.getValue(), pts.getValue(), pts.getTimeBase());
			mavf->setDuration(availableSamples, pts.getTimeBase());
			mavf->setSpeed(m_speed);
			mavf->setMTS({ pts, pts });
			m_outTime.addOffset(availableSamples * m_speed);
			//
			//HJFLogi("sonic read samples:{}, total samples intpu:{} output:{} speed:{}", avf->nb_samples, m_inTime.getValue(), m_outTime.getValue(), m_speed);
		}
		else if (m_eof) {
			mavf = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
		}
		else {
			HJFLogw("warning, sonic available samples:{}", availableSamples);
		}
	} while (false);

	return mavf;
}

HJBuffer::Ptr HJSTAudioProcessor::receiveSamples(int& sample)
{
	if (!m_soundtouch) {
		return nullptr;
	}
	int actualSamples = 0;
	int samplesize = m_info->m_channels * m_info->m_bytesPerSample;
	HJBuffer::Ptr buffer = std::make_shared<HJBuffer>(m_maxSamples * samplesize);
	uint8_t* pout = buffer->spaceData();
	//
	m_outputBufer.resize(m_info->m_channels * m_maxSamples * sizeof(float));
	float* fout = m_outputBufer.data();
	do
	{
		actualSamples = m_soundtouch->receiveSamples((soundtouch::SAMPLETYPE*)fout, (uint)m_maxSamples);
		if (actualSamples > 0) {
			floatToShort(fout, (short*)pout, m_info->m_channels, actualSamples);
			buffer->addSize(actualSamples * samplesize);
			//
			pout = buffer->spaceData();
			sample += actualSamples;
		}
	} while (actualSamples != 0);

	return (buffer->size() > 0) ? buffer : nullptr;
}

void HJSTAudioProcessor::reset()
{
	if (!m_soundtouch) {
		return;
	}
	m_soundtouch->clear();
	return;
}

int HJSTAudioProcessor::checkInfo(const HJAudioInfo::Ptr& ainfo)
{
	if (!m_soundtouch || (m_info->m_samplesRate != ainfo->m_samplesRate) || (m_info->m_channels != ainfo->m_channels)) {
		done();
		return init(ainfo);
	}
	return HJ_OK;
}

void HJSTAudioProcessor::updateParams()
{

}

void HJSTAudioProcessor::setSpeed(const float speed)
{
	if (!m_soundtouch) {
		return;
	}
	HJAudioProcessor::setSpeed(speed);

	m_soundtouch->setTempoChange((m_speed - 1) * 100.0);

	return;
}



NS_HJ_END