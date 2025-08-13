//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFrameParser.h"
#include "HJFFUtils.h"
#include "HJException.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJFrameParser::HJFrameParser(const HJBuffer::Ptr params, const int codecID)
{
    int res = init(params, codecID);
    if (res != HJ_OK) {
        HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "frame parser alloc failed");
        return;
    }
}
HJFrameParser::~HJFrameParser()
{
	done();
}

int HJFrameParser::init(const HJBuffer::Ptr params, const int codecID)
{
	if (!params) {
		return HJErrInvalidParams;
	}
	int res = HJ_OK;
	do
	{
        auto codec = avcodec_find_decoder((enum AVCodecID)codecID);
        if (!codec) {
            HJFLoge("find decoder failed, codec id:{}", codecID);
            res = HJErrCodecInit;
            break;
        }
        m_avctx = avcodec_alloc_context3(codec);
        if (!m_avctx) {
            HJFLoge("alloc context failed, codec id:{}", codecID);
            res = HJErrCodecInit;
            break;
        }
        m_avctx->extradata = (uint8_t*)av_malloc(params->size());
        m_avctx->extradata_size = params->size();
        memcpy(m_avctx->extradata, params->data(), params->size());
        //
        m_parser = av_parser_init(codec->id);
        if (!m_parser) {
            HJFLoge("parse init failed, codec id:{}", codecID);
            break;
        }
        m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
	} while (false);

	return res;
}

int HJFrameParser::parse(HJBuffer::Ptr inData)
{
    if (!inData || !m_parser || !m_avctx) {
        return HJErrInvalidParams;
    }
    int res = HJ_OK;
    uint8_t* data = NULL;
    int size = 0;
    do
    { 
        int ret = av_parser_parse2(
            m_parser, m_avctx,
            &data, &size,
            inData->data(), inData->size(),
            AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0
        );
		if (ret < 0) {
			HJFLoge("parse data failed, ret:{}", ret);
			res = HJErrParseData;
			break;
		}
        HJFLogi("pict_type:{}, key_frame:{}", m_parser->pict_type, m_parser->key_frame);
    } while (false);
    return res;
}

void HJFrameParser::done()
{
    if(m_parser){
        av_parser_close(m_parser);
        m_parser = NULL;
    }
    if (m_avctx) {
        avcodec_free_context(&m_avctx);
        m_avctx = NULL;
    }

}


NS_HJ_END