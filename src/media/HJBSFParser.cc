//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJBSFParser.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJBSFParser::HJBSFParser()
{
    
}

HJBSFParser::HJBSFParser(const AVCodecParameters* codecParams)
{
    int res = init("", codecParams);
    if (HJ_OK != res) {
        HJLogi("init error");
    }
}

HJBSFParser::HJBSFParser(const std::string bsfName, const AVCodecParameters* codecParams)
{
    int res = init(bsfName, codecParams);
    if (HJ_OK != res) {
        HJLogi("init error");
    }
}

HJBSFParser::~HJBSFParser()
{
    destroyBSFContext();
    if (m_avctx) {
        AVCodecContext* avctx = (AVCodecContext*)m_avctx;
        avcodec_free_context(&avctx);
        m_avctx = NULL;
    }
}

int HJBSFParser::init(const std::string bsfName, const AVCodecParameters* codecParams, HJOptions::Ptr opts)
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
        res = avcodec_parameters_to_context(m_avctx, codecParams);
        if (res < HJ_OK) {
            break;
        }
        if (AV_CODEC_ID_H264 == m_avctx->codec_id) {
            m_bsfName = "h264_mp4toannexb";
            res = parseH264ExtraData(m_avctx);
        } else if (AV_CODEC_ID_HEVC == m_avctx->codec_id) {
            m_bsfName = "hevc_mp4toannexb";
            res = parseH265ExtraData(m_avctx);
        } else {
            return HJErrNotSupport;
        }
        if (HJ_OK != res) {
            break;
        }
        if (!bsfName.empty()) {
            m_bsfName = bsfName;
        }
        m_bsf = createBSFContext(codecParams, opts);
        if (!m_bsf) {
            res = HJErrNewObj;
        }
    } while (false);
    
    return res;
}

HJBuffer::Ptr HJBSFParser::filter(HJBuffer::Ptr inData, HJOptions::Ptr opts)
{
    if (!inData) {
        return nullptr;
    }
    int res = HJ_OK;
    HJBuffer::Ptr outData = nullptr;
    AVPacket* pkt = hj_make_null_packet();
    do {
        if (opts) {
            AVBSFContext* bsf = createBSFContext(m_bsf->par_in, opts);
            if (!bsf) {
                HJLogi("error, create bsf context failed");
                return nullptr;
            }
            destroyBSFContext();
            m_bsf = bsf;
        }

        pkt->data = inData->data();
        pkt->size = (int)inData->size();
        pkt->flags |= AV_PKT_FLAG_KEY;
        res = av_bsf_send_packet(m_bsf, pkt);
        if (res < HJ_OK) {
            break;
        }
        res = av_bsf_receive_packet(m_bsf, pkt);
        if (res < HJ_OK) {
            break;
        }
        outData = std::make_shared<HJBuffer>(pkt->data, pkt->size);
    } while (false);
    
    if (pkt) {
        av_packet_free(&pkt);
    }
    return outData;
}

int HJBSFParser::parseH264ExtraData(AVCodecContext* avctx)
{
    int i;
    int ret;

    H264ParamSets ps;
    const PPS *pps = NULL;
    const SPS *sps = NULL;
    int is_avc = 0;
    int nal_length_size = 0;

    memset(&ps, 0, sizeof(ps));

    ret = ff_h264_decode_extradata(avctx->extradata, avctx->extradata_size,
        &ps, &is_avc, &nal_length_size, 0, avctx);
    if (ret < 0) {
        goto done;
    }

    for (i = 0; i < MAX_PPS_COUNT; i++) {
        if (ps.pps_list[i]) {
            pps = ps.pps_list[i];
            break;
        }
    }

    if (pps) {
        if (ps.sps_list[pps->sps_id]) {
            sps = ps.sps_list[pps->sps_id];
        }
    }

    if (pps && sps) {
        uint8_t *data = NULL;
        int data_size = 0;

        avctx->profile = ff_h264_get_profile(sps);
        avctx->level = sps->level_idc;

        if ((ret = h2645_ps_to_nalu(sps->data, (int)sps->data_size, &data, &data_size)) < 0) {
            goto done;
        }
        //ff_AMediaFormat_setBuffer(format, "csd-0", (void*)data, data_size);
        m_csd0 = std::make_shared<HJBuffer>(data, data_size);
        av_freep(&data);

        if ((ret = h2645_ps_to_nalu(pps->data, (int)pps->data_size, &data, &data_size)) < 0) {
            goto done;
        }
        //ff_AMediaFormat_setBuffer(format, "csd-1", (void*)data, data_size);
        m_csd1 = std::make_shared<HJBuffer>(data, data_size);
        av_freep(&data);
    } else {
        const int warn = is_avc & (avctx->codec_tag == MKTAG('a', 'v', 'c', '1') ||
            avctx->codec_tag == MKTAG('a', 'v', 'c', '2'));
        HJLoge("Could not extract PPS/SPS from extradata, warn:" + HJ2STR(warn ? AV_LOG_WARNING : AV_LOG_DEBUG));
        ret = HJErrFatal;
    }
done:
    ff_h264_ps_uninit(&ps);

    return ret;
}

int HJBSFParser::parseH265ExtraData(AVCodecContext* avctx)
{
    int i;
    int ret;

    HEVCParamSets ps;
    HEVCSEI sei;

    const HEVCVPS *vps = NULL;
    const HEVCPPS *pps = NULL;
    const HEVCSPS *sps = NULL;
    int is_nalff = 0;
    int nal_length_size = 0;

    uint8_t *vps_data = NULL;
    uint8_t *sps_data = NULL;
    uint8_t *pps_data = NULL;
    int vps_data_size = 0;
    int sps_data_size = 0;
    int pps_data_size = 0;

    memset(&ps, 0, sizeof(ps));
    memset(&sei, 0, sizeof(sei));

    ret = ff_hevc_decode_extradata(avctx->extradata, avctx->extradata_size,
                                   &ps, &sei, &is_nalff, &nal_length_size, 0, 1, avctx);
    if (ret < 0) {
        goto done;
    }

    for (i = 0; i < HEVC_MAX_VPS_COUNT; i++) {
        if (ps.vps_list[i]) {
            vps = ps.vps_list[i];
            break;
        }
    }

    for (i = 0; i < HEVC_MAX_PPS_COUNT; i++) {
        if (ps.pps_list[i]) {
            pps = ps.pps_list[i];
            break;
        }
    }

    if (pps) {
        if (ps.sps_list[pps->sps_id]) {
            sps = ps.sps_list[pps->sps_id];
        }
    }

    if (vps && pps && sps) {
        avctx->profile = sps->ptl.general_ptl.profile_idc;
        avctx->level   = sps->ptl.general_ptl.level_idc;

        if ((ret = h2645_ps_to_nalu(vps->data, vps->data_size, &vps_data, &vps_data_size)) < 0 ||
            (ret = h2645_ps_to_nalu(sps->data, sps->data_size, &sps_data, &sps_data_size)) < 0 ||
            (ret = h2645_ps_to_nalu(pps->data, pps->data_size, &pps_data, &pps_data_size)) < 0) {
            goto done;
        }
        m_csd0 = std::make_shared<HJBuffer>(vps_data_size + sps_data_size + pps_data_size);
        m_csd0->appendData(vps_data, vps_data_size);
        m_csd0->appendData(sps_data, sps_data_size);
        m_csd0->appendData(pps_data, pps_data_size);
    } else {
        const int warn = is_nalff & (avctx->codec_tag == MKTAG('h','v','c','1'));
        HJLoge("Could not extract VPS/PPS/SPS from extradata, warn:" + HJ2STR(warn ? AV_LOG_WARNING : AV_LOG_DEBUG));
        ret = HJErrFatal;
    }

done:
    ff_hevc_ps_uninit(&ps);

    av_freep(&vps_data);
    av_freep(&sps_data);
    av_freep(&pps_data);

    return ret;
}

int HJBSFParser::h2645_ps_to_nalu(const uint8_t *src, int src_size, uint8_t **out, int *out_size)
{
    int i;
    int ret = 0;
    uint8_t *p = NULL;
    static const uint8_t nalu_header[] = { 0x00, 0x00, 0x00, 0x01 };
    if (!out || !out_size) {
        return AVERROR(EINVAL);
    }
    uint32_t buf_len = sizeof(nalu_header) + src_size;
    p = (uint8_t *)av_malloc(buf_len);
    if (!p) {
        return AVERROR(ENOMEM);
    }
    *out = p;
    *out_size = buf_len;
    //
    if(m_avcHeader) {
        *(uint32_t *)p = buf_len;
    } else {
        memcpy(p, nalu_header, sizeof(nalu_header));
    }
    memcpy(p + sizeof(nalu_header), src, src_size);

    /* Escape 0x00, 0x00, 0x0{0-3} pattern */
    for (i = 4; i < *out_size; i++) {
        if (i < *out_size - 3 &&
            p[i + 0] == 0 &&
            p[i + 1] == 0 &&
            p[i + 2] <= 3) {
            uint8_t *newV;

            *out_size += 1;
            newV = (uint8_t *)av_realloc(*out, *out_size);
            if (!newV) {
                ret = AVERROR(ENOMEM);
                goto done;
            }
            *out = p = newV;

            i = i + 2;
            memmove(p + i + 1, p + i, *out_size - (i + 1));
            p[i] = 0x03;
        }
    }
done:
    if (ret < 0) {
        av_freep(out);
        *out_size = 0;
    }

    return ret;
}

int HJBSFParser::setupOptions(AVBSFContext* bsf, HJOptions::Ptr opts)
{
    int res = HJ_OK;

    if (opts->haveStorage("sei_user_data")) {
        std::string seiData = opts->getString("sei_user_data");
        av_opt_set(bsf->priv_data, "sei_user_data", seiData.c_str(), AV_OPT_SEARCH_CHILDREN);
    }

    return res;
}

AVBSFContext* HJBSFParser::createBSFContext(const AVCodecParameters* codecParams, HJOptions::Ptr opts)
{
    int res = HJ_OK;
    AVBSFContext* bsf = NULL;
    do
    {
        const AVBitStreamFilter* bsfilter = av_bsf_get_by_name(m_bsfName.c_str());
        if (!bsfilter) {
            res = HJErrNewObj;
            break;
        }
        res = av_bsf_alloc(bsfilter, &bsf);
        if (res < HJ_OK) {
            break;
        }
        if (opts) {
            setupOptions(bsf, opts);
        }
        res = avcodec_parameters_copy(bsf->par_in, codecParams);
        if (res < HJ_OK) {
            break;
        }
        res = av_bsf_init(bsf);
    } while (false);

    if (res < HJ_OK) {
        if (bsf) {
            av_bsf_free(&bsf);
            bsf = NULL;
        }
    }

    return bsf;
}

void HJBSFParser::destroyBSFContext()
{
    if (m_bsf) {
        av_bsf_free(&m_bsf);
        m_bsf = NULL;
    }
}

NS_HJ_END
