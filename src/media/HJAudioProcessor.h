//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJTicker.h"

typedef struct sonicStreamStruct sonicStreamStruct;
typedef sonicStreamStruct* sonicStream;

NS_HJ_BEGIN
//***********************************************************************************//
class HJAudioProcessor : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJAudioProcessor>;
	HJAudioProcessor();
	virtual ~HJAudioProcessor();

	int init(const HJAudioInfo::Ptr& info);
	void done();
	int addFrame(const HJMediaFrame::Ptr& frame);
	int getFrame(HJMediaFrame::Ptr& frame);
	void reset();
	//
	void setSpeed(const float speed) {
		m_speed = speed;
	}
	const float getSpeed() {
		return m_speed;
	}
	void setPitch(const float pitch) {
		m_pitch = pitch;
	}
	const float getPitch() {
		return m_pitch;
	}
	void setRate(const float rate) {
		m_rate = rate;
	}
	const float getRate() {
		return m_rate;
	}
	void setVolume(const float volume) {
		m_volume = volume;
	}
	const float getVolume() {
		return m_volume;
	}
	void setQuality(const int quality) {
		m_quality = quality;
	}
	const int getQuality() {
		return m_quality;
	}
private:
	void checkParams();
protected:
	HJAudioInfo::Ptr	m_info = nullptr;
	sonicStream			m_stream = NULL;
	float				m_speed = 1.0f;
	float				m_pitch = 1.0f;
	float				m_rate = 1.0f;
	float				m_volume = 1.0f;
	int					m_quality = 1;
	bool				m_eof = false;
	HJUtilTime			m_inTime = HJUtilTime::HJTimeNOTS;
	HJUtilTime			m_outTime = HJUtilTime::HJTimeNOTS;
	//HJAFrameBriefList	m_inAFrameBriefs;
};

NS_HJ_END

