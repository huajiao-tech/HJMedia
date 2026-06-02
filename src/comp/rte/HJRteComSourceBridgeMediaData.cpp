#include "HJRteComSource.h"
#include "HJFLog.h"
#include "libyuv.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN

HJRteComSourceBridgeMediaData::HJRteComSourceBridgeMediaData()
{
    HJ_SetInsName(HJRteComSourceBridgeMediaData);
    HJRteCom::setPriority(HJRteComPriority_VideoSource);
    m_textureType = HJRteTextureType_2D;
}
HJRteComSourceBridgeMediaData::~HJRteComSourceBridgeMediaData()
{
    HJFLogi("~HJRteComSourceBridgeMediaData");
    if (m_bCreateTexture)
    {
        HJOGCommon::textureDestroy(m_textureId);
        m_textureId = 0;
        m_bCreateTexture = false;
    }
}

int HJRteComSourceBridgeMediaData::update(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        if (!m_bCreateTexture)
        {
            m_bCreateTexture = true;
            m_textureId = HJOGCommon::textureCreate();
        }

        if (m_mediaDataGetCb)
        {
            HJTransferMediaData::Ptr mediaData = nullptr;
            m_mediaDataGetCb(mediaData);

            int width = 0;
            int height = 0;
            if (mediaData)
            {
                width = mediaData->getWidth();
                height = mediaData->getHeight();
                bool bUpdate = false;
                if (m_catchWidth != width || m_catchHeight != height)
                {
                    m_catchWidth = width;
                    m_catchHeight = height;
                    bUpdate = true;
                }

                HJTransferMediaData::Ptr rgba = HJTransferMediaData::create(mediaData, HJConvertDataType_RGBA);
                if (rgba)
                {
                    glBindTexture(GL_TEXTURE_2D, m_textureId);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->getWidth(), rgba->getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->getData(0));
                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                /*unsigned char* pRGBA = nullptr;
                HJTransferMediaDataYUVI420::Ptr mediaDataYUV420 = std::dynamic_pointer_cast<HJTransferMediaDataYUVI420>(mediaData);
                HJTransferMediaDataRGBA::Ptr mediaDataRGBA = std::dynamic_pointer_cast<HJTransferMediaDataRGBA>(mediaData);
                HJTransferMediaDataYUVNV12::Ptr mediaDataYUVNV12 = std::dynamic_pointer_cast<HJTransferMediaDataYUVNV12>(mediaData);

                if (mediaDataYUV420)
                { 
                    if (bUpdate)
                    {
                        m_RGBABuf = HJSPBuffer::create(m_catchWidth * m_catchHeight * 4);
                    }
                    libyuv::I420ToABGR(mediaDataYUV420->getData(0), mediaDataYUV420->getStride(0),
                        mediaDataYUV420->getData(1), mediaDataYUV420->getStride(1),
                        mediaDataYUV420->getData(2), mediaDataYUV420->getStride(2),
                        m_RGBABuf->getBuf(), m_catchWidth * 4, m_catchWidth, m_catchHeight);

                    pRGBA = m_RGBABuf->getBuf();
                }
                else if (mediaDataYUVNV12)
                {
                    if (bUpdate)
                    {
                        m_RGBABuf = HJSPBuffer::create(m_catchWidth * m_catchHeight * 4);
                    }
                    libyuv::NV12ToABGR(mediaDataYUVNV12->getData(0), mediaDataYUVNV12->getStride(0),
                        mediaDataYUVNV12->getData(1), mediaDataYUVNV12->getStride(1),
                        m_RGBABuf->getBuf(), m_catchWidth * 4, m_catchWidth, m_catchHeight);
                    pRGBA = m_RGBABuf->getBuf();
                }
                else if (mediaDataRGBA)
                {
                    pRGBA = mediaDataRGBA->getBuffer()->getBuf();
                }
                
                if (pRGBA)
                {
                    glBindTexture(GL_TEXTURE_2D, m_textureId);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pRGBA);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }  */
                m_bReady = true;
            }
            else
            {
                m_bReady = false;
            }
        }                    
    } while (false);
    return i_err;
}

int HJRteComSourceBridgeMediaData::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do
    {
        i_err = HJRteComSourceBridge::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        HJ_CatchMapGetVal(i_param, HJTransferMediaDataGetCb, m_mediaDataGetCb);      
    } while (false);
    return i_err;
}

int HJRteComSourceBridgeMediaData::setParam(HJBaseParam::Ptr i_param)
{
    HJ_CatchMapGetVal(i_param, HJTransferMediaDataGetCb, m_mediaDataGetCb);
    return HJ_OK;
}

bool HJRteComSourceBridgeMediaData::IsStateAvaiable()
{
    return IsStateReady();
}
bool HJRteComSourceBridgeMediaData::IsStateReady()
{
    return m_bReady;
}
int HJRteComSourceBridgeMediaData::getWidth()
{
    return m_catchWidth;
}
int HJRteComSourceBridgeMediaData::getHeight()
{
    return m_catchHeight;
}
float* HJRteComSourceBridgeMediaData::getTexMatrix()
{
    return HJOGCommon::TextureMatrixYFlip;
}
GLuint HJRteComSourceBridgeMediaData::getTextureId()
{
    return m_textureId;
}

void HJRteComSourceBridgeMediaData::done()
{  
    HJRteComSourceBridge::done();
}

NS_HJ_END
