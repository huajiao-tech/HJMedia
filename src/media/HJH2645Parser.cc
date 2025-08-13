//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJH2645Parser.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJH2645Parser::HJH2645Parser()
{
    
}

HJH2645Parser::HJH2645Parser(const AVCodecParameters* codecParams)
{
    if(codecParams) {
        init(codecParams);
    }
}

HJH2645Parser::~HJH2645Parser()
{
    done();
}

int HJH2645Parser::init(const AVCodecParameters* codecParams)
{
    if (!codecParams) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    do {
        m_avctx = avcodec_alloc_context3(NULL);
        if (!m_avctx) {
            res = HJErrFFNewAVCtx;
        }
        int ret = avcodec_parameters_to_context(m_avctx, codecParams);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            HJLogi("copy codec params error");
            break;
        }
//        std::string bsfName = "";
        if (AV_CODEC_ID_H264 == m_avctx->codec_id)
        {
            int is_avc = 1;
            int nal_length_size = 0;
            const PPS *pps = NULL;
            const SPS *sps = NULL;
            
            m_h264Params = (H264ParamSets *)av_mallocz(sizeof(H264ParamSets));
            
            ret = ff_h264_decode_extradata(m_avctx->extradata, m_avctx->extradata_size,
                                           m_h264Params, &is_avc, &nal_length_size, 0, m_avctx);
            if (ret < HJ_OK) {
                res = HJErrFatal;
                HJLogi("parse extra data error");
                break;
            }
            
            for (int i = 0; i < MAX_PPS_COUNT; i++) {
                if (m_h264Params->pps_list[i]) {
                    pps = m_h264Params->pps_list[i];
                    break;
                }
            }

            if (pps) {
                if (m_h264Params->sps_list[pps->sps_id]) {
                    sps = m_h264Params->sps_list[pps->sps_id];
                }
            }
            m_h264Params->pps = pps;
            m_h264Params->sps = sps;
//            bsfName = "h264_mp4toannexb";
        } else if (AV_CODEC_ID_HEVC == m_avctx->codec_id)
        {
            int is_nalff = 1;
            int nal_length_size = 0;
            const HEVCVPS *vps = NULL;
            const HEVCPPS *pps = NULL;
            const HEVCSPS *sps = NULL;
            
            m_h265Params = (HEVCParamSets *)av_mallocz(sizeof(HEVCParamSets));
            m_h265Sei = (HEVCSEI *)av_mallocz(sizeof(HEVCSEI));
            
            ret = ff_hevc_decode_extradata(m_avctx->extradata, m_avctx->extradata_size,
                                           m_h265Params, m_h265Sei, &is_nalff, &nal_length_size, 0, 1, m_avctx);
            if (ret < HJ_OK) {
                res = HJErrFatal;
                HJLogi("parse extra data error");
                break;
            }
            
            for (int i = 0; i < HEVC_MAX_VPS_COUNT; i++) {
                if (m_h265Params->vps_list[i]) {
                    vps = m_h265Params->vps_list[i];
                    break;
                }
            }

            for (int i = 0; i < HEVC_MAX_PPS_COUNT; i++) {
                if (m_h265Params->pps_list[i]) {
                    pps = m_h265Params->pps_list[i];
                    break;
                }
            }

            if (pps) {
                if (m_h265Params->sps_list[pps->sps_id]) {
                    sps = m_h265Params->sps_list[pps->sps_id];
                }
            }
            m_h265Params->vps = vps;
            m_h265Params->pps = pps;
            m_h265Params->sps = sps;
//            bsfName = "hevc_mp4toannexb";
        } else {
            res = HJErrNotSupport;
            HJLogi("not support, codec id:" + HJ2STR(m_avctx->codec_id));
            break;
        }
        setting();
        m_reboot = false;
#if 0
        const AVBitStreamFilter* bsfilter = av_bsf_get_by_name(bsfName.c_str());
        if (!bsfilter) {
            res = HJErrNewObj;
            break;
        }
        ret = av_bsf_alloc(bsfilter, &m_bsf);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
        ret = avcodec_parameters_copy(m_bsf->par_in, codecParams);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
        ret = av_bsf_init(m_bsf);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
#endif
    } while (false);
    
    return res;
}

int HJH2645Parser::parse(HJBuffer::Ptr inData)
{
    if (!inData || !inData->size()) {
        return HJErrInvalidParams;
    }
    return HJ_OK;
//    return parse(inData->data(), (int)inData->size(), isAVCHeader);
}

int HJH2645Parser::parse(const AVPacket* pkt)
{
    if (!pkt) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
//    AVPacket* pkt = NULL;
//    AVIOContext* pb = NULL;
    do {
#if 0
        pkt = hj_make_null_packet();
        pkt->data = data;
        pkt->size = size;
        int ret = av_bsf_send_packet(m_bsf, pkt);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
        ret = av_bsf_receive_packet(m_bsf, pkt);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
        //
        ret = avio_open_dyn_buf(&pb);
        if (ret < HJ_OK) {
            res = HJErrFatal;
            break;
        }
        if (AV_CODEC_ID_H264 == m_avctx->codec_id) {
            ff_isom_write_avcc(pb, pkt->data, pkt->size);
        } else if (AV_CODEC_ID_HEVC == m_avctx->codec_id) {
            ff_isom_write_hvcc(pb, pkt->data, pkt->size, 0);
        } else {
            res = HJErrNotSupport;
            break;
        }
        uint8_t* extradata_buf = NULL;
        int extradata_size = avio_get_dyn_buf(pb, &extradata_buf);
        if (!extradata_buf || !extradata_size) {
            break;
        }
#endif
        uint8_t* extradata_buf = NULL;
        size_t extradata_size = 0;
        extradata_buf = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, &extradata_size);
        if (!extradata_buf || !extradata_size) {
            break;
        }
        //
        if (AV_CODEC_ID_H264 == m_avctx->codec_id)
        {
            int is_avc = 0;
            int nal_length_size = 0;
            const PPS *pps = NULL;
            const SPS *sps = NULL;
            
            H264ParamSets* params = (H264ParamSets *)av_mallocz(sizeof(H264ParamSets));
            
            int ret = ff_h264_decode_extradata(extradata_buf, (int)extradata_size, params, &is_avc, &nal_length_size, 0, m_avctx);
            if (ret < HJ_OK) {
                av_freep(&params);
                res = HJErrESParse;
                break;
            }
            for (int i = 0; i < MAX_PPS_COUNT; i++) {
                if (params->pps_list[i]) {
                    pps = params->pps_list[i];
                    break;
                }
            }

            if (pps) {
                if (params->sps_list[pps->sps_id]) {
                    sps = params->sps_list[pps->sps_id];
                }
            }
            params->pps = pps;  //ff_refstruct_ref_c
            params->sps = sps;
            if (!params->pps || !params->sps) {
                break;
            }
            //
            reset();
            m_h264Params = params;
            m_exbuffer = std::make_shared<HJBuffer>(extradata_buf, extradata_size);
        }
        else if (AV_CODEC_ID_HEVC == m_avctx->codec_id)
        {
            int is_nalff = 0;
            int nal_length_size = 0;
            const HEVCVPS *vps = NULL;
            const HEVCPPS *pps = NULL;
            const HEVCSPS *sps = NULL;
            
            HEVCParamSets* params = (HEVCParamSets *)av_mallocz(sizeof(HEVCParamSets));
            HEVCSEI* sei = (HEVCSEI *)av_mallocz(sizeof(HEVCSEI));
            
            int ret = ff_hevc_decode_extradata(extradata_buf, (int)extradata_size, params, sei, &is_nalff, &nal_length_size, 0, 1, m_avctx);
            if (ret < HJ_OK) {
                av_freep(&params);
                av_freep(&sei);
                res = HJErrESParse;
                break;
            }
            for (int i = 0; i < HEVC_MAX_VPS_COUNT; i++) {
                if (params->vps_list[i]) {
                    vps = params->vps_list[i];
                    break;
                }
            }

            for (int i = 0; i < HEVC_MAX_PPS_COUNT; i++) {
                if (params->pps_list[i]) {
                    pps = params->pps_list[i];
                    break;
                }
            }

            if (pps) {
                if (params->sps_list[pps->sps_id]) {
                    sps = params->sps_list[pps->sps_id];
                }
            }
            params->vps = vps;
            params->pps = pps;
            params->sps = sps;
            if (!params->pps || !params->sps) {
                break;
            }
            //
            reset();
            m_h265Params = params;
            m_h265Sei = sei;
            m_exbuffer = std::make_shared<HJBuffer>(extradata_buf, extradata_size);
        }
        m_reboot = false;
        setting();
        if (m_reboot) {
            break;
        }
        if (extradata_size != m_avctx->extradata_size) {
            m_reboot = true;
            break;
        }
        bool same = (0 == memcmp(extradata_buf, m_avctx->extradata, m_avctx->extradata_size));
        if (!same) {
            m_reboot = true;
        }
    } while (false);
//    if (pkt) {
//        av_packet_free(&pkt);
//    }
//    if (pb) {
//        ffio_free_dyn_buf(&pb);
//    }
    //HJLogi("parse end");
    
    return res;
}

void HJH2645Parser::done()
{
    reset();
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
//    if (m_bsf) {
//        av_bsf_free(&m_bsf);
//        m_bsf = NULL;
//    }
}

void HJH2645Parser::reset()
{
    if (m_h264Params) {
        m_h264Params->pps = NULL;
        m_h264Params->sps = NULL;
        ff_h264_ps_uninit(m_h264Params);
        av_freep(&m_h264Params);
        m_h264Params = NULL;
    }
    if (m_h265Params) {
        m_h265Params->vps = NULL;
        m_h265Params->pps = NULL;
        m_h265Params->sps = NULL;
        ff_hevc_ps_uninit(m_h265Params);
        av_freep(&m_h265Params);
        m_h265Params = NULL;
    }
    if (m_h265Sei) {
        av_freep(&m_h265Sei);
        m_h265Sei = NULL;
    }
    m_exbuffer = nullptr;
}

void HJH2645Parser::setting()
{
    if (m_h264Params)
    {
        if (m_h264Params->sps)
        {
            HJSizei vsize = {16 * m_h264Params->sps->mb_width, 16 * m_h264Params->sps->mb_height};
            vsize.w -= (m_h264Params->sps->crop_left + m_h264Params->sps->crop_right);
            vsize.h -= (m_h264Params->sps->crop_top + m_h264Params->sps->crop_bottom);
            //
            m_reboot = false;
            if(vsize.w != m_vsize.w ||
               vsize.h != m_vsize.h ||
               m_profile != m_h264Params->sps->profile_idc ||
               m_level != m_h264Params->sps->level_idc )
            {
                m_vsize = vsize;
                m_profile = m_h264Params->sps->profile_idc;
                m_level = m_h264Params->sps->level_idc;
                m_reboot = true;
            }
        }
    } else if(m_h265Params)
    {
        if (m_h265Params->sps) {
            const HEVCWindow *ow;
            ow  = &m_h265Params->sps->output_window;
            HJSizei vsize = {m_h265Params->sps->width, m_h265Params->sps->height};
            vsize.w -= (ow->left_offset + ow->right_offset);
            vsize.h -= (ow->top_offset  + ow->bottom_offset);
            //
            m_reboot = false;
            if(vsize.w != m_vsize.w ||
               vsize.h != m_vsize.h ||
               m_profile != m_h265Params->sps->ptl.general_ptl.profile_idc ||
               m_level != m_h265Params->sps->ptl.general_ptl.level_idc )
            {
                m_vsize = vsize;
                m_profile  = m_h265Params->sps->ptl.general_ptl.profile_idc;
                m_level    = m_h265Params->sps->ptl.general_ptl.level_idc;
                m_reboot = true;
            }
        }
    }
}
NS_HJ_END
