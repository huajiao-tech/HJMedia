//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAudioConverter.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJAudioConverter::HJAudioConverter()
{
    m_dstInfo = std::make_shared<HJAudioInfo>();
	m_srcInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_dstInfo->dup());
    m_nipMuster = std::make_shared<HJNipMuster>();
	//m_buffer = std::make_shared<HJBuffer>(m_dstInfo->m_channels * m_dstInfo->m_samplesPerFrame * m_dstInfo->m_bytesPerSample + 256);
}

HJAudioConverter::HJAudioConverter(const HJAudioInfo::Ptr& info)
{
    m_dstInfo = std::dynamic_pointer_cast<HJAudioInfo>(info->dup());
    m_srcInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_dstInfo->dup());
    m_nipMuster = std::make_shared<HJNipMuster>();
    //m_buffer = std::make_shared<HJBuffer>(m_dstInfo->m_channels * m_dstInfo->m_samplesPerFrame * m_dstInfo->m_bytesPerSample + 256);
}

HJAudioConverter::~HJAudioConverter()
{
    if (m_swr) {
        SwrContext* swr = (SwrContext *)m_swr;
        swr_free(&swr);
        m_swr = NULL;
    }
}

HJMediaFrame::Ptr HJAudioConverter::convert(HJMediaFrame::Ptr&& inFrame)
{
    if (!m_dstInfo || !m_dstInfo->getChannelLayout()) {
        return std::move(inFrame);
    }
	//HJLogi("convert begin");
	HJAudioInfo::Ptr inInfo = std::dynamic_pointer_cast<HJAudioInfo>(inFrame->m_info);
    //if (inInfo->m_channels != m_srcInfo->m_channels) {
    //    inFrame = channelMap(std::move(inFrame), m_srcInfo->m_channels);
    //    //
    //    inInfo = std::dynamic_pointer_cast<HJAudioInfo>(inFrame->m_info);
    //    m_srcInfo->setChannels(inInfo->m_channels, inInfo->m_channelLayout, inInfo->getAVChannelLayout());
    //}
    AVFrame* avf = (AVFrame *)inFrame->getAVFrame();
    if (!avf) {
        HJLogi("have no AVFrame");
        return nullptr;
    }
    HJChannelLayout::Ptr tmpCHLayout = m_srcInfo->getChannelLayout();
    if (!tmpCHLayout){
        tmpCHLayout = std::make_shared<HJChannelLayout>(inInfo->m_channels);
    }
    HJChannelLayout::Ptr inCHLayout = inInfo->getChannelLayout();
    if (!inCHLayout) {
        HJLogi("in channel layout invalid");
        return nullptr;
    }
    //int64_t ch_layout = (inInfo->m_channelLayout && inInfo->m_channels == av_get_channel_layout_nb_channels(inInfo->m_channelLayout)) ? inInfo->m_channelLayout : av_get_default_channel_layout(inInfo->m_channels);
    
    if (av_channel_layout_compare((const AVChannelLayout *)tmpCHLayout->getAVCHLayout(), (const AVChannelLayout*)inCHLayout->getAVCHLayout()) ||
        m_srcInfo->m_sampleFmt != inInfo->m_sampleFmt ||
        m_srcInfo->m_samplesRate != inInfo->m_samplesRate )
    {
        if (m_swr) {
            SwrContext* ctx = (SwrContext *)(m_swr);
            swr_free(&ctx);
            m_swr = NULL;
        }
        m_srcInfo->setChannels(inInfo->m_channels, inCHLayout->getAVCHLayout());
        m_srcInfo->m_sampleFmt = inInfo->m_sampleFmt;
        m_srcInfo->m_samplesRate = inInfo->m_samplesRate;
        //
        SwrContext* swr = NULL;
        HJChannelLayout::Ptr dstCHLayout = m_dstInfo->getChannelLayout();
        HJChannelLayout::Ptr srcCHLayout = m_srcInfo->getChannelLayout();
        int ret = swr_alloc_set_opts2(&swr,
            (AVChannelLayout *)dstCHLayout->getAVCHLayout(), (AVSampleFormat)m_dstInfo->m_sampleFmt, m_dstInfo->m_samplesRate,
            (AVChannelLayout *)srcCHLayout->getAVCHLayout(), (AVSampleFormat)m_srcInfo->m_sampleFmt, m_srcInfo->m_samplesRate,
            0, NULL);
        //swr = swr_alloc_set_opts(
        //                        NULL,
        //                        m_dstInfo->m_channelLayout,
        //                        (AVSampleFormat)m_dstInfo->m_sampleFmt,
        //                        m_dstInfo->m_samplesRate,
        //                        m_srcInfo->m_channelLayout,
        //                        (AVSampleFormat)m_srcInfo->m_sampleFmt,
        //                        m_srcInfo->m_samplesRate,
        //                        0, NULL);
        if (!swr || swr_init(swr) < HJ_OK) {
            HJLoge("swr_init error");
            return NULL;
        }
        m_swr = swr;
    }
    
    HJMediaFrame::Ptr outFrame = nullptr;
    if (m_swr)
    {
        const uint8_t **in = { (const uint8_t **)avf->data };
        //
        int outSamples = (int)av_rescale_rnd(swr_get_delay((SwrContext *)m_swr, inInfo->m_samplesRate) + inInfo->m_sampleCnt, m_dstInfo->m_samplesRate, inInfo->m_samplesRate, AV_ROUND_UP);
//        int predCount = inInfo->m_sampleCnt * m_dstInfo->m_samplesRate / inInfo->m_samplesRate + 256;
        //int outSampleSize = av_samples_get_buffer_size(NULL, m_dstInfo->m_channels, outSamples, (AVSampleFormat)m_dstInfo->m_sampleFmt, 0);
        //if(av_sample_fmt_is_planar((AVSampleFormat)m_dstInfo->m_sampleFmt)){
        //    outSampleSize = av_samples_get_buffer_size(NULL, 1, outSamples, (AVSampleFormat)m_dstInfo->m_sampleFmt, 0);
        //}
        //m_buffer->setCapacity(outSampleSize);
        
        HJAudioInfo::Ptr outInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_dstInfo->dup());
        outInfo->m_sampleCnt = outSamples;
        HJMediaFrame::Ptr mavf = HJMediaFrame::makeSilenceAudioFrame(outInfo);
        if (!mavf) {
            HJLoge("make slience audio frame error");
            return nullptr;
        }
        AVFrame* outAvf = (AVFrame *)mavf->getAVFrame();
        if (!outAvf) {
            HJLoge("get avf error");
            return nullptr;
        }
        uint8_t** out = { (uint8_t**)outAvf->data };
        int nbSamples = swr_convert((SwrContext *)m_swr,
                                    out,
                                    outSamples,
                                    in,
                                    avf->nb_samples);
        if (nbSamples < 0) {
            HJLoge("swr_convert error, nbSamples:" + HJ2STR(nbSamples));
            return nullptr;
        }
        outInfo->m_sampleCnt = outAvf->nb_samples = nbSamples;
        //int nbBufSize =  av_samples_get_buffer_size(NULL, m_dstInfo->m_channels, nbSamples, (AVSampleFormat)m_dstInfo->m_sampleFmt, 0);
        //outFrame = HJMediaFrame::makeAudioFrame();
        //outFrame->setData(m_buffer->data(), nbBufSize);
        mavf->setPTSDTS(inFrame->m_pts, inFrame->m_dts, inFrame->m_timeBase);
        mavf->m_frameType = inFrame->m_frameType;
        outFrame = std::move(mavf);
        //
        HJNipInterval::Ptr nip = m_nipMuster->getOutNip();
        if (outFrame && nip && nip->valid()) {
            HJLogi(outFrame->formatInfo());
        }
        //HJLogi("202308181100 src { nb_samples: " + HJ2STR(avf->nb_samples) + ", total samples:" + HJ2STR(m_inSamplesCount) + ", pts: " + HJ2STR(avf->pts) + ", delta:" + HJ2STR(avf->pts - m_inSamplesCount) + " }, " +
        //    "dst { pred nb_samples : " + HJ2STR(outSamples) + ", get nb_samples : " + HJ2STR(nbSamples) + ", total samples:" + HJ2STR(m_outSamplesCount) + ", pts : " + HJ2STR(outAvf->pts) + ", delta:" + HJ2STR(outAvf->pts - m_outSamplesCount) + " }");
        //m_inSamplesCount += avf->nb_samples;
        //m_outSamplesCount += outAvf->nb_samples;
    } else
    {
        outFrame = inFrame;
    }
    //HJLogi("convert end");

    return outFrame;
}

HJMediaFrame::Ptr HJAudioConverter::channelMap(HJMediaFrame::Ptr&& inFrame, int channels)
{
	if (!m_dstInfo || !m_dstInfo->getChannelLayout()) {
		return std::move(inFrame);
	}
	AVFrame* avf = (AVFrame*)inFrame->getAVFrame();
	if (!avf) {
		HJLogi("have no AVFrame");
		return nullptr;
	}
	//HJLogi("channelMap begin");
	HJAudioInfo::Ptr inInfo = std::dynamic_pointer_cast<HJAudioInfo>(inFrame->m_info);
    //
    HJMediaFrame::Ptr outFrame = HJMediaFrame::makeAudioFrame();
	outFrame->setPTSDTS(inFrame->m_pts, inFrame->m_dts, inFrame->m_timeBase);
	outFrame->m_frameType = inFrame->m_frameType;
    //
    HJAudioInfo::Ptr outAInfo = outFrame->getAudioInfo();
    outAInfo->clone(inInfo);
    outAInfo->m_sampleCnt = inInfo->m_sampleCnt;
    outAInfo->setChannels(channels);
    //
    int res = HJ_OK;
    AVFrame* outAvf = NULL;
    do 
    {
        outAvf = hj_dup_audio_frame(avf, channels, (const AVChannelLayout*)outAInfo->getAVChannelLayout());
        if (!outAvf) {
            break;
        }
        switch (avf->format)
        {
        case AV_SAMPLE_FMT_FLTP: // 1 ch -> n ch; n ch -> 1 ch
        {
			for (int i = 0; i < channels; i++) {
				if (outAvf->data[i]) {
					memcpy(outAvf->data[i], avf->data[0], outAvf->linesize[0]);
				}
			}
            break;
        }
        default:
            break;
        }
        outFrame->setMFrame(outAvf);
    } while (false);

	if (HJ_OK != res && outAvf) {
		av_frame_free(&outAvf);
        outAvf = NULL;
	}
    //HJLogi("channelMap end");

    return std::move(outFrame);
}

//const std::string HJAudioAdopter::FILTERS_DESC = "[in]volume=volume=1.0[out]";
const std::string HJAudioAdopter::FILTERS_DESC = "[in]aresample[out]";
//***********************************************************************************//
HJAudioAdopter::HJAudioAdopter()
{
}

HJAudioAdopter::HJAudioAdopter(const HJAudioInfo::Ptr& info)
{
    m_dstInfo = info;
    m_srcInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_dstInfo->dup());
}

HJAudioAdopter::~HJAudioAdopter()
{
    done();
}

int HJAudioAdopter::init()
{
    int res = HJ_OK;
    HJLogi("init begin");
    m_nipMuster = std::make_shared<HJNipMuster>();
	AVFilterGraph* avGraph = avfilter_graph_alloc();
	if (!avGraph) {
		return HJErrNewObj;
	}
	m_avGraph = avGraph;
	avGraph->nb_threads = 1;
	//
	AVFilterInOut* inputs = NULL;
	AVFilterInOut* outputs = NULL;
	do {
		std::string filterDesc = HJAudioAdopter::FILTERS_DESC;
		HJLogi("init avfilter graph desc:" + filterDesc + ", name:" + getName());
		int res = avfilter_graph_parse2(avGraph, filterDesc.c_str(), &inputs, &outputs);
		if (res < HJ_OK) {
			HJLoge("avfilter_graph_parse2 filter desc error");
			break;
		}
		for (AVFilterInOut* input = inputs; input; input = input->next)
		{
			AVBPrint args;
			av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
//			av_bprintf(&args, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx",
//				1, m_srcInfo->m_samplesRate, m_srcInfo->m_samplesRate,
//				av_get_sample_fmt_name((AVSampleFormat)m_srcInfo->m_sampleFmt), m_srcInfo->m_channelLayout);  //ff7 deprecated
			const std::string filterName = HJMakeGlobalName("abuffer");
			//
			AVFilterContext* filter = NULL;
			res = avfilter_graph_create_filter(&filter, avfilter_get_by_name("abuffer"),
				filterName.c_str(), args.str, NULL,
				avGraph);
			if (res < HJ_OK || !filter) {
				HJLoge("filter create error");
				res = HJErrFatal;
				break;
			}
			res = avfilter_link(filter, 0, input->filter_ctx, input->pad_idx);
			if (res < HJ_OK) {
				HJLoge("avfilter_link error:" + HJ2STR(res));
				res = HJErrFatal;
				break;
			}
			//
			m_inFilters.emplace(input->name, filter);
		}

		HJChannelLayout::Ptr chLayout = m_dstInfo->getChannelLayout();
		if (!chLayout) {
			res = HJErrInvalidParams;
			break;
		}
		m_timebase = { 1, m_dstInfo->m_samplesRate };
        //
		for (AVFilterInOut* output = outputs; output; output = output->next)
		{
			const std::string filterName = HJMakeGlobalName("abuffersink");
			//
			AVFilterContext* filter = NULL;
			res = avfilter_graph_create_filter(&filter, avfilter_get_by_name("abuffersink"),
				filterName.c_str(), NULL, NULL,
				avGraph);
			if (res < HJ_OK || !filter) {
				HJLoge("filter create error");
				res = HJErrFatal;
				break;
			}
			AVSampleFormat sample_fmt = (AVSampleFormat)m_dstInfo->m_sampleFmt;
			res = av_opt_set_bin(filter, "sample_fmts",
				(uint8_t*)&sample_fmt, sizeof(sample_fmt),
				AV_OPT_SEARCH_CHILDREN);
			if (res < HJ_OK) {
				HJLoge("abuffersink set opt error");
				break;
			}
			char ch_buf[64];
			av_channel_layout_describe((const AVChannelLayout*)chLayout->getAVCHLayout(), ch_buf, sizeof(ch_buf));
			res = av_opt_set(filter, "ch_layouts", ch_buf, AV_OPT_SEARCH_CHILDREN);
			if (res < HJ_OK) {
				HJLoge("abuffersink set opt error");
				break;
			}

			res = av_opt_set_bin(filter, "sample_rates",
				(uint8_t*)&m_dstInfo->m_samplesRate, sizeof(m_dstInfo->m_samplesRate),
				AV_OPT_SEARCH_CHILDREN);
			if (res < HJ_OK) {
				HJLoge("abuffersink set opt error");
				break;
			}

			res = avfilter_link(output->filter_ctx, output->pad_idx, filter, 0);
			if (res < HJ_OK) {
				HJLoge("avfilter_link error:" + HJ2STR(res));
				res = HJErrFatal;
				break;
			}
			//
			m_outFilters.emplace(output->name, filter);
		}

		res = avfilter_graph_config(avGraph, NULL);
		if (res < HJ_OK) {
			HJLoge("avfilter_graph_config error:" + HJ2STR(res));
			res = HJErrFatal;
			break;
		}
#if defined(DEBUG) || defined(_DEBUG)
		char* dump = avfilter_graph_dump(avGraph, NULL);
		if (dump) {
			HJLogi("graph :\n" + std::string(dump));
		    av_free(dump);
		}
#endif
	} while (false);

	if (inputs) {
		avfilter_inout_free(&inputs);
	}
	if (outputs) {
		avfilter_inout_free(&outputs);
	}
    HJLogi("init end");

    return res;
}

void HJAudioAdopter::done()
{
    if (m_avGraph) {
        AVFilterGraph* avGraph = (AVFilterGraph*)m_avGraph;
        avfilter_graph_free(&avGraph);
        m_avGraph = nullptr;
    }
    m_inFilters.clear();
    m_outFilters.clear();
}

HJMediaFrame::Ptr HJAudioAdopter::convert(HJMediaFrame::Ptr&& frame)
{
    if (!m_dstInfo || !m_dstInfo->getChannelLayout()) {
        return std::move(frame);
    }
    //HJLogi("convert begin");
    HJAudioInfo::Ptr inInfo = std::dynamic_pointer_cast<HJAudioInfo>(frame->m_info);
    HJChannelLayout::Ptr inCHLayout = inInfo->getChannelLayout();
    if (!inCHLayout) {
        HJLogi("in channel layout invalid");
        return nullptr;
    }
    HJChannelLayout::Ptr tmpCHLayout = m_srcInfo->getChannelLayout();
    if (!tmpCHLayout) {
        tmpCHLayout = std::make_shared<HJChannelLayout>(inInfo->m_channels);
    }
    int res = HJ_OK;
    if (av_channel_layout_compare((const AVChannelLayout*)tmpCHLayout->getAVCHLayout(), (const AVChannelLayout*)inCHLayout->getAVCHLayout()) ||
        m_srcInfo->m_sampleFmt != inInfo->m_sampleFmt ||
        m_srcInfo->m_samplesRate != inInfo->m_samplesRate)
    {
        m_srcInfo->setChannels(inInfo->m_channels, inCHLayout->getAVCHLayout());
        m_srcInfo->m_sampleFmt = inInfo->m_sampleFmt;
        m_srcInfo->m_samplesRate = inInfo->m_samplesRate;

        done();
        res = init();
        if (HJ_OK != res) {
            return nullptr;
        }
    }
    HJMediaFrame::Ptr outFrame = nullptr;
    if (m_avGraph)
    {
        res = addFrame(std::move(frame));
        if (HJ_OK != res) {
            return nullptr;
        }
        outFrame = getFrame();
    } else {
        outFrame = std::move(frame);
    }
    //HJLogi("convert end");

    return outFrame;
}

int HJAudioAdopter::addFrame(HJMediaFrame::Ptr&& frame)
{
    if (!m_avGraph) {
        return HJErrNotAlready;
    }
    AVFilterContext* inFilter = (AVFilterContext*)m_inFilters.begin()->second;
    if (!frame || !inFilter) {
        HJLoge("addFrame frame error");
        return HJErrInvalidParams;
    }
    frame->setAVTimeBase(m_timebase);

    AVFrame* avf = (AVFrame*)frame->getAVFrame();
    if (!avf) {
        return HJErrInvalidParams;
    }
    int res = av_buffersrc_write_frame(inFilter, avf);
    if (AVERROR(EAGAIN) == res) {
        return HJ_WOULD_BLOCK;
    }
    else if (AVERROR_EOF == res) {
        return HJ_EOF;
    }
    else if (res < HJ_OK) {
        return HJErrFatal;
    }
    if (res < HJ_OK) {
        HJLoge("addFrame end, res:" + HJ2STR(res));
    }
    return res;
}

HJMediaFrame::Ptr HJAudioAdopter::getFrame()
{
    AVFilterContext* outFilter = (AVFilterContext*)m_outFilters.begin()->second;
    if (!outFilter) {
        HJLoge("get out filter error");
        return nullptr;
    }
    int res = HJ_OK;
    AVFrame* avf = av_frame_alloc();
    if (!avf) {
        return nullptr;
    }
    HJMediaFrame::Ptr outFrame = nullptr;
    do {
        res = av_buffersink_get_frame(outFilter, avf);
        if (AVERROR(EAGAIN) == res) {
            res = HJ_WOULD_BLOCK;
            break;
        }
        else if (AVERROR_EOF == res) {
            outFrame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
            res = HJ_EOF;
            break;
        }
        else if (res < HJ_OK) {
            HJLoge("receive frame error:" + HJ2String(res));
            res = HJErrFFGetFrame;
            break;
        }
        else {
            HJMediaFrame::Ptr mvf = HJMediaFrame::makeAudioFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            avf->best_effort_timestamp = avf->pts;
            avf->time_base = av_rational_from_hj_rational(m_timebase);
            mvf->setPTSDTS(avf->best_effort_timestamp, avf->best_effort_timestamp, m_timebase);
            if ((avf->key_frame && avf->pict_type && (avf->pict_type == AV_PICTURE_TYPE_I))) {
                mvf->setFrameType(HJFRAME_KEY);
            }
            //
            HJAudioInfo::Ptr audioInfo = std::dynamic_pointer_cast<HJAudioInfo>(mvf->m_info);
            AVChannelLayout* avchLayout = NULL;
            HJChannelLayout::Ptr chLayout = m_dstInfo->getChannelLayout();
            if (chLayout) {
                avchLayout = (AVChannelLayout*)chLayout->getAVCHLayout();
            }
            audioInfo->setChannels(avchLayout->nb_channels, avchLayout);
            audioInfo->m_sampleFmt = m_dstInfo->m_sampleFmt;
            audioInfo->m_bytesPerSample = av_get_bytes_per_sample((AVSampleFormat)audioInfo->m_sampleFmt);
            audioInfo->m_samplesRate = avf->sample_rate;
            audioInfo->m_sampleCnt = avf->nb_samples;
            audioInfo->m_samplesPerFrame = m_dstInfo->m_samplesPerFrame;
            audioInfo->m_blockAlign = m_dstInfo->m_blockAlign;
            //
            HJNipInterval::Ptr nip = m_nipMuster->getOutNip();
            if (mvf && nip && nip->valid()) {
                HJLogi(mvf->formatInfo());
            }
            mvf->setMFrame(avf);
            avf = nullptr;
            //
            outFrame = std::move(mvf);
        }
    } while (false);

    if (avf) {
        av_frame_free(&avf);
    }
    return outFrame;
}


NS_HJ_END
