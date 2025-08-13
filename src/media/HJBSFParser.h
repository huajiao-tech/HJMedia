//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVBSFContext AVBSFContext;

NS_HJ_BEGIN
//***********************************************************************************//
/*
 * insert sps/pps/vps for key frame
 */
class HJBSFParser : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBSFParser>;
    HJBSFParser();
    HJBSFParser(const AVCodecParameters* codecParams);
    HJBSFParser(const std::string bsfName, const AVCodecParameters* codecParams);
    virtual ~HJBSFParser();

    /**
     * h264_mp4toannexb
     * hevc_mp4toannexb
     * h264_metadata
     * hevc_metadata
     */
    int init(const std::string bsfName, const AVCodecParameters* codecParams, HJOptions::Ptr opts = nullptr);
    HJBuffer::Ptr filter(HJBuffer::Ptr inData, HJOptions::Ptr opts = nullptr);
    //
    const HJBuffer::Ptr& getCSD0() {
        return m_csd0;
    }
    const HJBuffer::Ptr& getCSD1() {
        return m_csd1;
    }
    void setAVCHeader(const bool avcHeader) {
        m_avcHeader = avcHeader;
    }
    const bool getAVCHeader() {
        return m_avcHeader;
    }
private:
    int parseH264ExtraData(AVCodecContext* avctx);
    int parseH265ExtraData(AVCodecContext* avctx);
    int h2645_ps_to_nalu(const uint8_t *src, int src_size, uint8_t **out, int *out_size);

    int setupOptions(AVBSFContext* bsf, HJOptions::Ptr opts);
    AVBSFContext* createBSFContext(const AVCodecParameters* codecParams, HJOptions::Ptr opts);
    void destroyBSFContext();
private:
    std::string         m_bsfName = "";
    AVCodecContext*     m_avctx = NULL;
    HJBuffer::Ptr      m_csd0 = nullptr;
    HJBuffer::Ptr      m_csd1 = nullptr;
    AVBSFContext*       m_bsf = NULL;
    bool                m_avcHeader = false;
};

NS_HJ_END

