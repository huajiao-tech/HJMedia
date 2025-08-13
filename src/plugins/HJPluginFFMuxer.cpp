#pragma once

#include "HJPluginFFMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJPluginFFMuxer::HJPluginFFMuxer(const std::string& i_name, size_t i_identify)
	: HJPluginMuxer(i_name, i_identify) { 
	m_muxer = std::make_shared<HJFFMuxer>();
	m_muxer->setName(getName());
}

void HJPluginFFMuxer::afterInit()
{
	m_status = HJSTATUS_Ready;
	HJPluginMuxer::afterInit();
}

void HJPluginFFMuxer::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		int rate{};
		if (i_mediaFrame->isAudio()) {
			auto info = i_mediaFrame->getAudioInfo();
			sample_size += info->getSampleCnt();
			if ((now - start_time) / 1000 >= duration_count * 5) {
				auto samples = duration_count * 5 * info->getSampleRate();
				HJFLogi("{}, ({})s, samples({}), sample_size diff({}), delay({})",
					getName(), duration_count * 5, samples, sample_size - samples, now - i_mediaFrame->getPTS());
				duration_count++;
			}
		}
		else if (i_mediaFrame->isVideo()) {
			auto info = i_mediaFrame->getVideoInfo();
			frame_size++;
			if ((now - start_time) / 1000 >= v_duration_count * 5) {
				auto frames = v_duration_count * 5 * info->m_frameRate;
				HJFLogi("{}, ({})s, frames({}), sample_size diff({}), delay({})",
					getName(), v_duration_count * 5, frames, frame_size - frames, now - i_mediaFrame->getPTS());
				v_duration_count++;
			}
		}
	}
	else {
		start_time  = now;
	}
}

void HJPluginFFMuxer::onWriteFrame(HJMediaFrame::Ptr& io_outFrame)
{
	io_outFrame = io_outFrame->deepDup();

	if (m_tsOffset == HJ_INT64_MAX) {
		m_tsOffset = -io_outFrame->getDTS();
	}

	io_outFrame->setPTSDTS(io_outFrame->getPTS() + m_tsOffset, io_outFrame->getDTS() + m_tsOffset);
}

NS_HJ_END
