//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJVideoConverter.h"
#include "HJLog.h"
#include "HJFFUtils.h"
#include "libyuv.h"

NS_HJ_BEGIN
const std::string HJVideoConverter::FILTERS_DESC = "[in]scale_cuda=360:640:interp_algo=bilinear[out]";
//***********************************************************************************//
HJVideoConverter::HJVideoConverter(const HJVideoInfo::Ptr& info)
{
    m_outInfo = info;
}

HJVideoConverter::~HJVideoConverter()
{
    
}

int HJVideoConverter::init(const HJVideoInfo::Ptr& info)
{
    if (!m_outInfo) {
        return HJErrNotAlready;
    }
    HJLogi("init entry");
   
    int res = HJ_OK;
    AVBufferSrcParameters* inpar = av_buffersrc_parameters_alloc();
	if (!inpar) {
		return AVERROR(ENOMEM);
	}
    do {
        m_inInfo = info;
        AVFilterGraph* avGraph = avfilter_graph_alloc();
        if (!avGraph) {
            res = HJErrNewObj;
            break;
        }
        m_avGraph = avGraph;
        
        AVFilterInOut* inputs = NULL;
        AVFilterInOut* outputs = NULL;
        std::string filterDesc = formatFilterDesc();
        if (filterDesc.empty()) {
            filterDesc = HJVideoConverter::FILTERS_DESC;
        }
        res = avfilter_graph_parse2(avGraph, HJVideoConverter::FILTERS_DESC.c_str(), &inputs, &outputs);
        if (res < HJ_OK) {
            HJLogi("avfilter graph parse2 error");
            break;
        }
        for (AVFilterInOut* input = inputs; input; input = input->next)
        {
            AVBPrint args;
            av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
            av_bprintf(&args, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                m_inInfo->m_width, m_inInfo->m_height,
                m_inInfo->m_pixFmt, 1, m_inInfo->m_frameRate,
                m_inInfo->m_aspectRatio.num, m_inInfo->m_aspectRatio.den);
            const std::string filterName = HJMakeGlobalName("buffer");
            //
            AVFilterContext* filter = NULL;
            res = avfilter_graph_create_filter(&filter, avfilter_get_by_name("buffer"),
                filterName.c_str(), args.str, NULL,
                avGraph);
            if (res < HJ_OK || !filter) {
                HJLoge("filter create error");
                break;
            }

            memset(inpar, 0, sizeof(*inpar));
            inpar->format = m_inInfo->m_pixFmt;//AV_PIX_FMT_NONE;
            if (m_hwFrameCtx) {
                inpar->hw_frames_ctx = av_buffer_ref((AVBufferRef*)m_hwFrameCtx->getHWFrameRef());
                if (!inpar->hw_frames_ctx) {
                    HJLoge("error, get hw frame ctx failed!");
                    break;
                }
            }
			res = av_buffersrc_parameters_set(filter, inpar);
            if (res < HJ_OK) {
                HJLogi("buffer set params error");
                break;
            }

            res = avfilter_link(filter, 0, input->filter_ctx, input->pad_idx);
            if (res < HJ_OK) {
                HJLoge("avfilter_link error:" + HJ2STR(res));
                break;
            }
            //
            m_inFilters.push_back(filter);
        }
        
        enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_CUDA, AV_PIX_FMT_NV12, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
        for (AVFilterInOut* output = outputs; output; output = output->next)
        {
            const std::string filterName = HJMakeGlobalName("buffersink");
            //
            AVFilterContext* filter = NULL;
            res = avfilter_graph_create_filter(&filter, avfilter_get_by_name("buffersink"),
                                               filterName.c_str(), NULL, NULL,
                                               avGraph);
            if (res < HJ_OK || !filter) {
                HJLoge("filter create error");
                break;
            }
            res = av_opt_set_int_list(filter, "pix_fmts", pix_fmts,
                                      AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
            
            res = avfilter_link(output->filter_ctx, output->pad_idx, filter, 0);
            if (res < HJ_OK) {
                HJLoge("avfilter_link error:" + HJ2STR(res));
                break;
            }
            //
            m_outFilters.push_back(filter);
        }
        
        res = avfilter_graph_config(avGraph, NULL);
        if (res < HJ_OK) {
            HJLoge("avfilter_graph_config error:" + HJ2STR(res));
            break;
        }
        char* dump = avfilter_graph_dump(avGraph, NULL);
        if (dump) {
            HJLogi("graph :\n" + std::string(dump));
        }
    } while(false);

    if (inpar) {
        av_freep(&inpar);
    }

    HJLogi("init end");

    return HJ_OK;
}

int HJVideoConverter::getFrame(HJMediaFrame::Ptr& frame)
{
    if (m_outFilters.size() <= 0) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    AVFrame* avf = av_frame_alloc();
    if (!avf) {
        return HJErrNewObj;
    }
    do {
        AVFilterContext* outFilter = (AVFilterContext *)m_outFilters[0];
        res = av_buffersink_get_frame(outFilter, avf);
        if (AVERROR(EAGAIN) == res) {
            res = HJ_WOULD_BLOCK;
            break;
        } else if(AVERROR_EOF == res) {
            frame = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_VIDEO);
//            int nb_reqs = getFailedRequests();
            res = HJ_EOF;
            break;
        } else if(res < HJ_OK) {
            HJLoge("receive frame error:" + HJ2String(res));
            res = HJErrFFGetFrame;
            break;
        } else {
            HJMediaFrame::Ptr mvf = HJMediaFrame::makeVideoFrame();
            if (!mvf) {
                res = HJErrNewObj;
                break;
            }
            mvf->m_pts = avf->best_effort_timestamp;
            mvf->m_dts = avf->best_effort_timestamp;
            if((avf->key_frame && avf->pict_type && (avf->pict_type == AV_PICTURE_TYPE_I))) {
                mvf->setFrameType(HJFRAME_KEY);
            }
            //
            HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(mvf->m_info);
            videoInfo->m_width = avf->width;
            videoInfo->m_height = avf->height;
            videoInfo->m_stride[0] = avf->linesize[0];
            videoInfo->m_stride[1] = avf->linesize[1];
            videoInfo->m_stride[2] = avf->linesize[2];
            videoInfo->m_stride[3] = avf->linesize[3];
            videoInfo->m_pixFmt = avf->format;
            //
            mvf->setMFrame(avf);
            avf = nullptr;
            //
            frame = std::move(mvf);
        }
    } while (false);
    
    if (avf) {
        av_frame_free(&avf);
    }
    return res;
}

int HJVideoConverter::addFrame(const HJMediaFrame::Ptr& frame)
{
    if (!frame) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    AVFrame* avf = nullptr;
    if(frame && (HJFRAME_EOF != frame->getFrameType())) {
        avf = frame->getAVFrame();
    }
    if (!avf) {
        avf = (AVFrame *)frame->makeAVFrame();
    }
    HJLogi("addFrame begin, pts:" + HJ2STR(frame->getPTS()));
    AVFilterContext* inFilter = (AVFilterContext *)m_inFilters[0];
    if (!inFilter) {
        return HJErrNotAlready;
    }
    res = av_buffersrc_write_frame(inFilter, avf);
    if (AVERROR(EAGAIN) == res) {
        return HJ_WOULD_BLOCK;
    } else if(AVERROR_EOF == res) {
        return HJ_EOF;
    } else if (res < HJ_OK){
        return HJErrFatal;
    }
    return res;
}

HJMediaFrame::Ptr HJVideoConverter::convert(const HJMediaFrame::Ptr& inFrame)
{
    int res = HJ_OK;
    if (!m_avGraph)
    {
        AVFrame* avf = (AVFrame*)inFrame->getAVFrame();
        if (avf && avf->hw_frames_ctx) {
            m_hwFrameCtx = std::make_shared<HJHWFrameCtx>(avf->hw_frames_ctx);
        }
        HJVideoInfo::Ptr videoInfo = std::dynamic_pointer_cast<HJVideoInfo>(inFrame->m_info);
        res = init(videoInfo);
        if (HJ_OK != res) {
            return nullptr;
        }
    }
    res = addFrame(inFrame);
    if (HJ_OK != res) {
        return nullptr;
    }
    HJMediaFrame::Ptr outFrame = nullptr;
    res = getFrame(outFrame);
    if (res < HJ_OK) {
        return nullptr;
    }
    return outFrame;
}

//"[in]scale_cuda=360:640:interp_algo=bilinear[out]"
std::string HJVideoConverter::formatFilterDesc()
{
    std::string filterDesc = "";
    AVBPrint args;
    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    av_bprintf(&args, "[in]scale=%d:%d[out]",
        m_inInfo->m_width, m_inInfo->m_height,
        m_inInfo->m_pixFmt, 1, m_inInfo->m_frameRate,
        m_inInfo->m_aspectRatio.num, m_inInfo->m_aspectRatio.den);
    filterDesc = HJ2SSTR(args.str);

    return filterDesc;
}

//***********************************************************************************//
HJVSwsConverter::HJVSwsConverter(const HJVideoInfo::Ptr& outInfo)
{
    m_outInfo = outInfo;
}

HJVSwsConverter::~HJVSwsConverter()
{
    done();
}

int HJVSwsConverter::init(const HJVideoInfo::Ptr& inInfo)
{
    if (!m_outInfo) {
        return HJErrNotAlready;
    }
    HJLogi("init entry, rotate:" + HJ2STR(inInfo->m_rotate));
    
    int res = HJ_OK;
    do
    {
        m_inInfo = std::dynamic_pointer_cast<HJVideoInfo>(inInfo->dup());
        //m_vcropMode = cropMode;
        //
        m_isRotate = (90 == m_inInfo->m_rotate || 270 == m_inInfo->m_rotate);
        m_srsRect = {0, 0, static_cast<float>(m_inInfo->m_width), static_cast<float>(m_inInfo->m_height)};
        m_dstRect = {0, 0, static_cast<float>(m_outInfo->m_width), static_cast<float>(m_outInfo->m_height)};
        if(m_isRotate) {
            m_dstRect = {0, 0, static_cast<float>(m_outInfo->m_height), static_cast<float>(m_outInfo->m_width)};
        }
        HJSizei inSize = {m_inInfo->m_width, m_inInfo->m_height};
        HJSizei outSize = {m_outInfo->m_width, m_outInfo->m_height};
        if(m_isRotate) {
            outSize = {m_outInfo->m_height, m_outInfo->m_width};
        }
        if(HJ_VCROP_FIT == m_vcropMode) {
            m_dstRect = HJMediaUtils::calculateRenderRect(outSize, inSize);
        } else if(HJ_VCROP_CLIP == m_vcropMode) {
            m_srsRect = HJMediaUtils::calculateRenderRect(inSize, outSize);
        } else if(HJ_VCROP_FILL == m_vcropMode){
            //nothing to do
        } else {
            res = HJErrNotSupport;
            break;
        }
        //x, y, w, h -- align 2
        m_srsRect.x = ((int)m_srsRect.x) & 0xFFFFFFFE;
        m_srsRect.y = ((int)m_srsRect.y) & 0xFFFFFFFE;
        m_srsRect.w = ((int)m_srsRect.w) & 0xFFFFFFFE;
        m_srsRect.h = ((int)m_srsRect.h) & 0xFFFFFFFE;
        m_dstRect.x = ((int)m_dstRect.x) & 0xFFFFFFFE;
        m_dstRect.y = ((int)m_dstRect.y) & 0xFFFFFFFE;
        m_dstRect.w = ((int)m_dstRect.w) & 0xFFFFFFFE;
        m_dstRect.h = ((int)m_dstRect.h) & 0xFFFFFFFE;
        
        if (!sws_isSupportedInput((enum AVPixelFormat)m_inInfo->m_pixFmt) ||
            !sws_isSupportedOutput((enum AVPixelFormat)m_outInfo->m_pixFmt)) {
            return HJErrNewObj;
        }
        m_sws = sws_getContext(m_srsRect.w, m_srsRect.h, (enum AVPixelFormat)m_inInfo->m_pixFmt,
                               m_dstRect.w, m_dstRect.h, (enum AVPixelFormat)m_outInfo->m_pixFmt,
            SWS_BILINEAR, NULL, NULL, NULL);
        if (!m_sws) {
            res = HJErrNewObj;
            break;
        }
    } while (false);

    return res;
}

void HJVSwsConverter::done()
{
    if (m_sws) {
        sws_freeContext(m_sws);
        m_sws = NULL;
    }
}

HJMediaFrame::Ptr HJVSwsConverter::convert(const HJMediaFrame::Ptr& inFrame)
{
    if (!inFrame) {
        return nullptr;
    }
    int res = HJ_OK;
    HJVideoInfo::Ptr inInfo = std::dynamic_pointer_cast<HJVideoInfo>(inFrame->m_info);
    if (!m_sws || m_inInfo->m_width != inInfo->m_width || m_inInfo->m_height != inInfo->m_height || m_inInfo->m_pixFmt != inInfo->m_pixFmt)
    { 
        done();
        res = init(inInfo);
        if (HJ_OK != res) {
            return nullptr;
        }
    }
    AVFrame* outAvf = NULL;
    do
    {
        AVFrame* inAvf = (AVFrame*)inFrame->getAVFrame();
        if (!inAvf) {
            HJLoge("error, inframe get avframe is null");
            break;
        }
        if (m_outInfo->m_width != inAvf->width || m_outInfo->m_height != inAvf->height || m_outInfo->m_pixFmt != inAvf->format) {
            outAvf = scaleFrame(inAvf);
        } else {
            outAvf = inAvf;
        }
        if (!outAvf) {
            res = HJErrEINVAL;
            break;
        }
        //rotate
        if(0.0f != m_inInfo->m_rotate) 
        {
            AVFrame* avf = rotateFrame(outAvf);
            if (!avf) {
                res = HJErrEINVAL;
                break;
            }
            if (outAvf) {
                av_frame_free(&outAvf);
            }
            outAvf = avf;
        }
        //crop
        if(m_outInfo->m_width != outAvf->width || m_outInfo->m_height != outAvf->height) 
        {
            AVFrame* avf = cropFrame(outAvf);
            if (!avf) {
                res = HJErrEINVAL;
                break;
            }
            if (outAvf) {
                av_frame_free(&outAvf);
            }
            outAvf = avf;
        }
        //
        res = av_frame_copy_props(outAvf, inAvf);
        if (res < HJ_OK) {
            HJLoge("error, av_frame_copy_props res:" + HJ2STR(res));
            break;
        }
        //
        inFrame->setMFrame(outAvf);
        inAvf = NULL;
        outAvf = NULL;
#if HJ_HAVE_TRACKER
        inFrame->addTrajectory(HJakeTrajectory());
#endif
    } while (false);

    if (outAvf) {
        av_frame_free(&outAvf);
    }
    return inFrame;
}

/**
* with dup frame
*/
HJMediaFrame::Ptr HJVSwsConverter::convert2(const HJMediaFrame::Ptr& inFrame)
{
    if (!inFrame) {
        return nullptr;
    }
    int res = HJ_OK;
    HJVideoInfo::Ptr inInfo = std::dynamic_pointer_cast<HJVideoInfo>(inFrame->m_info);
    if (!m_sws || m_inInfo->m_width != inInfo->m_width || m_inInfo->m_height != inInfo->m_height || m_inInfo->m_pixFmt != inInfo->m_pixFmt) 
    {
        done();
        res = init(inInfo);
        if (HJ_OK != res) {
            return nullptr;
        }
    }
    HJMediaFrame::Ptr outFrame = inFrame->dup();  //
    AVFrame* outAvf = NULL;
    do
    {
        AVFrame* inAvf = (AVFrame*)inFrame->getAVFrame();
        if (!inAvf) {
            HJLoge("error, inframe get avframe is null");
            break;
        }
        if (m_outInfo->m_width != inAvf->width || m_outInfo->m_height != inAvf->height || m_outInfo->m_pixFmt != inAvf->format) {
            outAvf = scaleFrame(inAvf);
        }
        else {
            outAvf = inAvf;
        }
        if (!outAvf) {
            res = HJErrEINVAL;
            break;
        }
        //rotate
        if (0.0f != m_inInfo->m_rotate) 
        {
            AVFrame* avf = rotateFrame(outAvf);
            if (!avf) {
                res = HJErrEINVAL;
                break;
            }
            if (outAvf) {
                av_frame_free(&outAvf);
            }
            outAvf = avf;
        }
        //crop
        //int stride = outAvf->linesize[0] * 8 / HJ_MIN(8, hj_get_bits_per_pixel(outAvf->format));
        if (m_outInfo->m_width != outAvf->width || m_outInfo->m_height != outAvf->height)
        {
            AVFrame* avf = cropFrame(outAvf);
            if (!avf) {
                res = HJErrEINVAL;
                break;
            }
            if (outAvf) {
                av_frame_free(&outAvf);
            }
            outAvf = avf;
        }
        //
        res = av_frame_copy_props(outAvf, inAvf);
        if (res < HJ_OK) {
            HJLoge("error, av_frame_copy_props res:" + HJ2STR(res));
            break;
        }
        //
        outFrame->setMFrame(outAvf);
        inAvf = NULL;
        outAvf = NULL;
#if HJ_HAVE_TRACKER
        outFrame->addTrajectory(HJakeTrajectory());
#endif
    } while (false);

    if (outAvf) {
        av_frame_free(&outAvf);
    }
    return outFrame;
}

#if 1
AVFrame* HJVSwsConverter::scaleFrame(const AVFrame* inAvf)
{
    int res = HJ_OK;
    AVFrame* outAvf = NULL;
    do {
        outAvf = av_frame_alloc();
        if (!outAvf) {
            HJLoge("error, alloc avframe failed");
            break;
        }
    #if 0
        outAvf->width = m_dstRect.w;
        outAvf->height = m_dstRect.h;
        outAvf->format = m_outInfo->m_pixFmt;
        res = av_frame_get_buffer(outAvf, 0);
        if(res < 0) {
            HJLoge("error, avframe get buffer failed, res:" + HJ2STR(res));
            break;
        }
        uint8_t* srcSlice[AV_NUM_DATA_POINTERS];
        uint8_t* dstSlice[AV_NUM_DATA_POINTERS];
        hj_get_avframe_slice(srcSlice, inAvf, m_srsRect, false);
        hj_get_avframe_slice(dstSlice, outAvf, m_dstRect, true);
        res = sws_scale(m_sws, srcSlice, inAvf->linesize, m_srsRect.y, m_srsRect.h, dstSlice, outAvf->linesize);
    #else
        //uint64_t t0 = HJUtils::getCurrentMicrosecond();
        res = sws_scale_frame(m_sws, outAvf, inAvf);
        //uint64_t t1 = HJUtils::getCurrentMicrosecond();
        //HJLogi("I420Scale time:" + HJ2STR(t1 - t0));
    #endif
        if (res < HJ_OK) {
            HJLoge("error, sws scale frame res:" + HJ2STR(res));
            break;
        }
        return outAvf;
    } while (false);
    
    if (outAvf) {
        av_frame_free(&outAvf);
    }
    return NULL;
}
#else
AVFrame* HJVSwsConverter::scaleFrame(const AVFrame* inAvf)
{
    int res = HJ_OK;
    AVFrame* outAvf = NULL;
    AVFrame* clrAvf = NULL;
    do {
        outAvf = hj_make_avframe(m_dstRect.w, m_dstRect.h, (enum AVPixelFormat)inAvf->format);
        if (!outAvf) {
            HJLoge("error, alloc avframe failed");
            break;
        }
        uint64_t t0 = HJUtils::getCurrentMicrosecond();
        res = libyuv::I420Scale(
            inAvf->data[0], inAvf->linesize[0],
            inAvf->data[1], inAvf->linesize[1],
            inAvf->data[2], inAvf->linesize[2],
            inAvf->width, inAvf->height,
            outAvf->data[0], outAvf->linesize[0],
            outAvf->data[1], outAvf->linesize[1],
            outAvf->data[2], outAvf->linesize[2],
            outAvf->width, outAvf->height,
            libyuv::kFilterBilinear
        );
        if (res < HJ_OK) {
            HJLoge("error, sws scale frame res:" + HJ2STR(res));
            break;
        }
        if (m_outInfo->m_pixFmt != outAvf->format)
        {
            clrAvf = hj_make_avframe(outAvf->width, outAvf->height, (enum AVPixelFormat)m_outInfo->m_pixFmt);
            if (!clrAvf) {
                HJLoge("error, alloc avframe failed");
                break;
            }
            res = libyuv::I420ToRGBA(
                outAvf->data[0], outAvf->linesize[0],
                outAvf->data[1], outAvf->linesize[1],
                outAvf->data[2], outAvf->linesize[2],
                clrAvf->data[0], clrAvf->linesize[0],
                clrAvf->width, clrAvf->height
            );
            if (res < HJ_OK) {
                HJLoge("error,I420ToRGBA frame res:" + HJ2STR(res));
                break;
            }
            av_frame_free(&outAvf);
            outAvf = clrAvf;
            clrAvf = NULL;
        }
        uint64_t t1 = HJUtils::getCurrentMicrosecond();
        HJLogi("I420Scale time:" + HJ2STR(t1 - t0));
        return outAvf;
    } while (false);

    if (outAvf) {
        av_frame_free(&outAvf);
    }
    if (clrAvf) {
        av_frame_free(&clrAvf);
    }
    return NULL;
}
#endif

AVFrame* HJVSwsConverter::rotateFrame(const AVFrame* inAvf)
{
    int res = HJ_OK;
    AVFrame* outAvf = NULL;
    do {
        int width = m_isRotate ? inAvf->height : inAvf->width;
        int height =  m_isRotate ? inAvf->width : inAvf->height;
        outAvf = hj_make_avframe(width, height, (enum AVPixelFormat)inAvf->format);
        if (!outAvf) {
            HJLoge("error, alloc avframe failed");
            break;
        }
        libyuv::RotationModeEnum rotMode = libyuv::kRotate0;
        if(90.0f == m_inInfo->m_rotate) {
            rotMode = libyuv::kRotate90;
        } else if (180.0f == m_inInfo->m_rotate) {
            rotMode = libyuv::kRotate180;
        } else if (270.0f == m_inInfo->m_rotate) {
            rotMode = libyuv::kRotate270;
        }
#if !defined(HJ_OS_DARWIN)
        if (AV_PIX_FMT_YUV420P == inAvf->format) {
            res = libyuv::I420Rotate(inAvf->data[0], inAvf->linesize[0], inAvf->data[1], inAvf->linesize[1], inAvf->data[2], inAvf->linesize[2],
                outAvf->data[0], outAvf->linesize[0], outAvf->data[1], outAvf->linesize[1], outAvf->data[2], outAvf->linesize[2],
                inAvf->width, inAvf->height, rotMode);
        }
        else {
            res = libyuv::ARGBRotate(inAvf->data[0], inAvf->linesize[0],
                outAvf->data[0], outAvf->linesize[0],
                inAvf->width, inAvf->height, rotMode
            );
        }
#endif
        if(res < HJ_OK) {
            HJLoge("error, libyuv I420Rotate failed, res:" + HJ2STR(res));
            break;
        }
        return outAvf;
    } while (false);
    
    if(outAvf) {
        av_frame_free(&outAvf);
    }
    return NULL;
}

AVFrame* HJVSwsConverter::cropFrame(const AVFrame* inAvf)
{
    int res = HJ_OK;
    AVFrame* outAvf = NULL;
    do {
        outAvf = hj_make_avframe(m_outInfo->m_width, m_outInfo->m_height, (enum AVPixelFormat)inAvf->format);
        if (!outAvf) {
            HJLoge("error, alloc avframe failed");
            break;
        }
        HJRectf targetRect = m_dstRect;
        if(m_isRotate) {
            targetRect = {m_dstRect.y, m_dstRect.x, m_dstRect.h, m_dstRect.w};
        }
        uint8_t* dstSlice[AV_NUM_DATA_POINTERS];
        hj_get_avframe_slice(dstSlice, outAvf, targetRect, true);
        av_image_copy((uint8_t **)dstSlice, outAvf->linesize, (const uint8_t **)inAvf->data, inAvf->linesize, (enum AVPixelFormat)inAvf->format, inAvf->width, inAvf->height);
        
        return outAvf;
    }while (false);
    //
    if(outAvf) {
        av_frame_free(&outAvf);
    }
    return NULL;
}

NS_HJ_END
