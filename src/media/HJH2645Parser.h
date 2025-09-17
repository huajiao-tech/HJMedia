//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseParser.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVPacket AVPacket;
typedef struct AVCodecContext AVCodecContext;
typedef struct H264ParamSets H264ParamSets;
typedef struct HEVCParamSets HEVCParamSets;
typedef struct HEVCSEI HEVCSEI;

typedef struct HEVCVPS HEVCVPS;
typedef struct HEVCPPS HEVCPPS;
typedef struct HEVCSPS HEVCSPS;

NS_HJ_BEGIN
//***********************************************************************************//
/*
 * reboot decoder or not
 * parser es frame, extract sps/pps/vps
 */
class HJH2645Parser : public HJBaseParser
{
public:
    using Ptr = std::shared_ptr<HJH2645Parser>;
    HJH2645Parser();
    HJH2645Parser(const AVCodecParameters* codecParams);
    virtual ~HJH2645Parser();
    
    virtual int init(const AVCodecParameters* codecParams);
    virtual int parse(HJBuffer::Ptr inData);
    virtual int parse(const AVPacket* pkt);
    virtual void done();
    
    const HJBuffer::Ptr& getExBuffer() {
        return m_exbuffer;
    }
    HJSizei getVSize() {
        return m_vsize;
    }
    int getProfile() {
        return m_profile;
    }
    int getLevel() {
        return m_level;
    }
    bool getReboot() {
        return m_reboot;
    }
private:
    void setting();
    void reset();
protected:
    AVCodecContext*     m_avctx = NULL;
    H264ParamSets*      m_h264Params = NULL;
    HEVCParamSets*      m_h265Params = NULL;
    HEVCSEI*            m_h265Sei = NULL;
//    AVBSFContext*       m_bsf = NULL;
    HJBuffer::Ptr      m_exbuffer = nullptr;
    //
    HJSizei            m_vsize = HJ_SIZE_ZERO;
    int                 m_profile = 0;
    int                 m_level = 0;
    bool                m_reboot = false;
    //
    HEVCVPS* m_vps = NULL;
    HEVCPPS* m_pps = NULL;
    HEVCSPS* m_sps = NULL;
};

//***********************************************************************************//


NS_HJ_END
