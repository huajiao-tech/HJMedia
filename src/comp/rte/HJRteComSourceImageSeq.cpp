#include "HJRteComSource.h"
#include "HJFLog.h"
#include "HJImgSeqInfo.h"

NS_HJ_BEGIN

HJRteComSourceImgSeq::HJRteComSourceImgSeq()
{
    HJ_SetInsName(HJRteComSourceImgSeq);
    HJRteCom::setPriority(HJRteComPriority_Gift);
    m_textureType = HJRteTextureType_2D;
}
HJRteComSourceImgSeq::~HJRteComSourceImgSeq()
{
    HJFLogi("~HJRteComSourceImgSeq");
}
int HJRteComSourceImgSeq::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComSource::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        m_parse = HJImgSeqParse::Create();
        m_parse->setNotify(m_notify);
        i_err = m_parse->init(i_param);
        if (i_err < 0)
        {
            break;
        }        
    } while (false);
    return i_err; 
}

void HJRteComSourceImgSeq::done()
{
    m_parse = nullptr;
    HJRteComSource::done();
}

int HJRteComSourceImgSeq::update(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do
    {
        if (!m_parse)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }
        i_err = m_parse->update();
        if (i_err < 0)
        {
            break;
        }    
    } while (false);
    return i_err; 
}
int HJRteComSourceImgSeq::getWidth() 
{
    if (!m_parse)
    {
        return 0;
    }
    return m_parse->getWidth();
}
int HJRteComSourceImgSeq::getHeight() 
{
    if (!m_parse)
    {
        return 0;
    }
    return m_parse->getHeight();
}
GLuint HJRteComSourceImgSeq::getTextureId() 
{
    if (!m_parse)
    {
        return 0;
    }
    return m_parse->getTextureId();
}
float* HJRteComSourceImgSeq::getTexMatrix()
{
    return HJOGCommon::TextureMatrixYFlip;
}
bool HJRteComSourceImgSeq::IsStateReady() 
{
    if (!m_parse)
    {
        return false;
    }
    return m_parse->IsStateReady();
}

HJRectf HJRteComSourceImgSeq::getRect()
{
    if (!m_parse)
    {
        return HJ_RECT_DEFAULT;
    }
    const HJImgSeqConfigPosInfo& pos = m_parse->getPositionInfo();
    return HJRectf{(float)pos.topx, (float)pos.topy, (float)pos.width, (float)pos.height};
}
NS_HJ_END