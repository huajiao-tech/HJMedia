#pragma once
#include "HJUtilitys.h"
#include "HJBuffer.h"
#include "HJStatisticalTools.h"

typedef struct AVStream AVStream;
typedef struct AVPacket AVPacket;

NS_HJ_BEGIN
//***********************************************************************************//
class HJPacketChecker :  public HJObject
{
public:
	using Ptr = std::shared_ptr<HJPacketChecker>;
	HJPacketChecker();
	virtual ~HJPacketChecker();

	int checkVideoPacket(AVStream* stream, AVPacket* pkt, int64_t pts, int64_t dts);
	int checkAudioPacket(AVStream* stream, AVPacket* pkt, int64_t pts, int64_t dts);
	void reset();
	//
	int getStreamID() const { return m_streamID; }
    std::pair<int64_t, int64_t> getVStats() const {
        return m_vstats;
    }
    std::pair<int64_t, int64_t> getAStats() const {
        return m_astats;
    }
    int64_t getSyncDelta();
    void setGapThreshold(int64_t threshold) { m_gapThreshold = threshold; }
private:
    void resetTS();
private:
	HJBuffer::Ptr	m_extraVideoBuffer{ nullptr };
	HJBuffer::Ptr	m_extraAudioBuffer{ nullptr };
    int64_t         m_lastVideoPTS{ HJ_NOTS_VALUE };
    int64_t         m_lastAudioPTS{ HJ_NOTS_VALUE };
	int64_t			m_lastVideoDTS{ HJ_NOTS_VALUE };
    int64_t			m_lastAudioDTS{ HJ_NOTS_VALUE };
	int64_t			m_gapThreshold{ 500 };
	int				m_streamID{ 0 };
    //
    HJBitrateTracker::Ptr m_vtracker;
    HJBitrateTracker::Ptr m_atracker;
    std::pair<int64_t, int64_t> m_vstats{0, 0};
    std::pair<int64_t, int64_t> m_astats{0, 0};
};



NS_HJ_END
