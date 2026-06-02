#include "HJRteComSource.h"
#include "HJFLog.h"
#include "HJRteUtils.h"
#include "HJMediaUtils.h"
#include "stb_image.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN

HJRteComSourceImage::HJRteComSourceImage()
{
    HJ_SetInsName(HJRteComSourceImage);
    HJRteCom::setPriority(HJRteComPriority_ImageSource);
    m_textureType = HJRteTextureType_2D;
}
HJRteComSourceImage::~HJRteComSourceImage()
{

}
int HJRteComSourceImage::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComSource::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        std::string url = "";
        HJ_CatchMapPlainGetVal(i_param, std::string, "ImageUrl", url);
        if (url.empty())
        {
            HJFLogw("{} ImageUrl is empty", m_insName);
            break;
        }
        //HJ_CatchMapGetVal(i_param, HJRectf, m_rect);

        int width = 0, height = 0, nrComponents = 0;
        unsigned char* data = stbi_load(url.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            m_width = width;
            m_height = height;

            m_texture = HJOGCommon::textureCreate();
            HJOGCommon::textureUpload(m_texture, width, height, nrComponents, data);
            stbi_image_free(data);
            m_bTextureReady = true;
        }
        else
        {
            i_err = HJErrInvalidFile;
            HJFLoge("{} load image failed, error:{}", m_insName, url);
            break;
        }
    } while (false);
    return i_err;
}
void HJRteComSourceImage::done()
{
    if (m_bTextureReady)
    {
        HJOGCommon::textureDestroy(m_texture);
        m_bTextureReady = false;
    }
    HJRteComSource::done();
}

int HJRteComSourceImage::update(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        
    } while (false);
    return i_err;
}

int HJRteComSourceImage::getWidth()
{
    return m_width;
}
int HJRteComSourceImage::getHeight()
{
    return m_height;
}
GLuint HJRteComSourceImage::getTextureId()
{  
    return m_texture;
}
bool HJRteComSourceImage::IsStateReady()
{
    return m_bTextureReady;
}
float* HJRteComSourceImage::getTexMatrix()
{
    return HJOGCommon::TextureMatrixYFlip;
}

//HJRectf HJRteComSourceImage::getRect()
//{
//    return m_rect;
//}


NS_HJ_END