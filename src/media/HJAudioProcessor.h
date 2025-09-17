//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJTicker.h"
#include "HJBuffer.h"

#define HJ_HAVE_SONIC			1
#define HJ_HAVE_SOUNDTOUCH		1
//#define HJ_HAVE_BUNGEE			1
//#define HJ_HAVE_RUBBERBAND		1

#if defined(HJ_HAVE_SONIC)
typedef struct sonicStreamStruct sonicStreamStruct;
typedef sonicStreamStruct* sonicStream;
#endif

#if defined(HJ_HAVE_SOUNDTOUCH)
namespace soundtouch {
	class SoundTouch;
}
#endif

#if defined(HJ_HAVE_BUNGEE)
#include "bungee/Bungee.h"
#include "bungee/Push.h"
#include "bungee/Stream.h"

namespace Bungee {
	struct Basic;
	template <class Edition>
	struct Stretcher;
	struct Request;

	template <class Implementation>
	class Stream;

	namespace Push {
		struct InputBuffer;
	}
}
#endif

#if defined(HJ_HAVE_RUBBERBAND)
namespace RubberBand {
	class RubberBandStretcher;
}
#endif

typedef struct AVFrame AVFrame;

NS_HJ_BEGIN
//***********************************************************************************//
//0: sonic 1:soundtouch 2:Bungee 3:RubberBand;
typedef enum HJAudioProcessorType {
	HJAudioProcessorSonic = 0,
	HJAudioProcessorSoundTouch = 1,
	HJAudioProcessorBungee = 2,
	HJAudioProcessorRubberBand = 3,
} HJAudioProcessorType;

class HJAudioProcessor : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJAudioProcessor);
	HJAudioProcessor() = default;
	virtual ~HJAudioProcessor() {};

	virtual int init(const HJAudioInfo::Ptr& info);
	virtual void done() = 0;
	virtual int addFrame(const HJMediaFrame::Ptr& frame) = 0;
	virtual HJMediaFrame::Ptr getFrame() = 0;
	virtual void reset() = 0;
	//
	//HJDeclareFunction(float, speed);
	virtual void setSpeed(const float speed) {
		m_speed = speed;
	}
	virtual const float getSpeed() const {
		return m_speed;
	}
    HJDeclareFunction(float, pitch);
    HJDeclareFunction(float, rate);
    HJDeclareFunction(float, volume);
    HJDeclareFunction(int, quality);
	//
    HJDeclareFunction(bool, eof);
public:
	static HJAudioProcessor::Ptr create(HJAudioProcessorType type = HJAudioProcessorSonic);
protected:
	int updateTime(AVFrame* avf);
	void shortToFloat(const short* input, float* output, int channels, int samples);
	void floatToShort(const float* input, short* output, int channels, int samples);
protected:
	HJAudioProcessorType m_type{ HJAudioProcessorSonic };
	HJAudioInfo::Ptr	 m_info{ nullptr };
	HJDeclareVariable(float, speed, 1.0f);
    HJDeclareVariable(float, pitch, 1.0f);
    HJDeclareVariable(float, rate, 1.0f);
    HJDeclareVariable(float, volume, 1.0f);
    HJDeclareVariable(int, quality, 1);

	//float				m_speed{ 1.0f };
	//float				m_pitch{ 1.0f };
	//float				m_rate{ 1.0f };
	//float				m_volume{ 1.0f };
	//int				m_quality{ 1 };
	bool				m_eof{ false };
	HJUtilTime			m_inTime = HJUtilTime::HJTimeNOTS;
	HJUtilTime			m_outTime = HJUtilTime::HJTimeNOTS;
};

//***********************************************************************************//
class HJSonicAudioProcessor : public HJAudioProcessor
{
public:
	HJ_DECLARE_PUWTR(HJSonicAudioProcessor);
	HJSonicAudioProcessor();
	virtual ~HJSonicAudioProcessor();

	virtual int init(const HJAudioInfo::Ptr& info) override;
	virtual void done() override;
	virtual int addFrame(const HJMediaFrame::Ptr& frame) override;
	virtual HJMediaFrame::Ptr getFrame() override;
	virtual void reset() override;
	//
	virtual void setSpeed(const float speed) override;
private:
	virtual void updateParams();
private:
	sonicStream			m_stream{ NULL };
};

//***********************************************************************************//
class HJSTAudioProcessor : public HJAudioProcessor
{
public:
	HJ_DECLARE_PUWTR(HJSTAudioProcessor);
	HJSTAudioProcessor();
	virtual ~HJSTAudioProcessor();

	virtual int init(const HJAudioInfo::Ptr& info) override;
	virtual void done() override;
	virtual int addFrame(const HJMediaFrame::Ptr& frame) override;
	virtual HJMediaFrame::Ptr getFrame() override;
	virtual void reset() override;
	//
	virtual void setSpeed(const float speed) override;
private:
	virtual int checkInfo(const HJAudioInfo::Ptr& ainfo);
	virtual void updateParams();
	HJBuffer::Ptr receiveSamples(int& sample);
private:
	std::shared_ptr<soundtouch::SoundTouch> m_soundtouch{nullptr};
	bool m_speech = false;
	std::vector<float>  m_inputBufer;
	std::vector<float>  m_outputBufer;
	int m_maxSamples = 4 * 1024;
};


NS_HJ_END