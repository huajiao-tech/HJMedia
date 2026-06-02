#include "HJImgSeqInfo.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJComUtils.h"
#include "HJComEvent.h"
#include "stb_image.h"
#include "HJOGBaseShader.h"
#include "HJOGCommon.h"
#include "HJRteUtils.h"

NS_HJ_BEGIN

void HJImgSeqParse::priDone()
{
    if (m_threadTimer)
    {
        m_threadTimer->stopTimer();
        m_threadTimer = nullptr;
    }    

    if (m_bTextureReady)
    {
        HJOGCommon::textureDestroy(m_texture);
        m_bTextureReady = false;
    }
}

HJImgSeqParse::~HJImgSeqParse()
{
    priDone();
}
std::vector<std::string> HJImgSeqParse::parseConfig(const std::string& i_path, HJImgSeqConfigInfo& o_configInfo)
{
    std::vector<std::string> urls;
    do
    {
        std::string configUrl = HJBaseUtils::combineUrl(i_path, "config.json");
        if (configUrl.empty())
        {
            HJFLoge("configUrl empty error");
            break;
        }
        std::string config = HJBaseUtils::readFileToString(configUrl);
        if (config.empty())
        {
            HJFLoge("config empty error");
            break;
        }
        int i_err = o_configInfo.deserial(config);
        if (i_err < 0)
        {
            HJFLoge("config json parse error");
            break;
        }
        HJFLogi("config enter loop:{} fps:{} prefix:{}",  o_configInfo.loops, o_configInfo.fps, o_configInfo.prefix);
        urls = HJBaseUtils::getSortedFiles(i_path, o_configInfo.prefix);
    } while (false);
    return urls;
}
int HJImgSeqParse::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        HJFLogi("{} init enter", m_insName);

        HJ_CatchMapGetVal(i_param, HJRenderIndexCb, m_idxNotify);

        std::string path = "";
        if (i_param->contains(HJRteUtils::ParamUrlImgSeq))
        {
            path = i_param->getValue<std::string>(HJRteUtils::ParamUrlImgSeq);
        }
        if (path.empty())
        {
            i_err = -1;
            break;
        }
        
        std::vector<std::string> urls = parseConfig(path, m_configInfo);
        if (urls.empty())
        {
            i_err = -1;
            break;
        }

        m_pngUrlQueue = std::move(urls);   
        m_pngIdx = 0;

//        m_configInfo.setloops(1);
//        m_configInfo.setfps(10);
        
        if (m_pngUrlQueue.empty())
        {
            i_err = -1;
            break;
        }    
        m_threadTimer = HJTimerThreadPool::Create();
        m_threadTimer->startTimer(1000/m_configInfo.fps, [this]{
            //decode every png
            if (m_bEnd)
            {
                return 0;
            }
            if (m_pngIdx == (int)m_pngUrlQueue.size())
            {
                m_pngIdx = 0;
                m_pngLoopIdx++;
                if (m_pngLoopIdx >= m_configInfo.loops)
                {
                    m_bEnd = true;
                    if (m_notify)
                    {
                        m_notify(HJBaseNotifyInfo::Create(HJVIDEORENDERGRAPH_EVENT_PNGSEQ_COMPLETE));
                    }    
                    HJFLogi("end, not proc");
                    return 0;
                }    
            }
            
            std::string url = m_pngUrlQueue[m_pngIdx];
            int width = 0, height = 0, nrComponents = 0;
		    unsigned char *data = stbi_load(url.c_str(), &width, &height, &nrComponents, 0);
            if (data)
            {
                HJRawImageDataInfo::Ptr rawInfo = HJRawImageDataInfo::Create();
                rawInfo->m_imgIdx = m_pngIdx;
                rawInfo->m_width = width;
                rawInfo->m_height = height;
                rawInfo->m_components = nrComponents;
                rawInfo->m_buffer = HJSPBuffer::create(width * height * nrComponents, data);
                m_cache.enqueue(rawInfo);
                //HJFLogi("{} decode png end idx:{} loop:{} fps:{} prefix:{}", m_insName, m_pngIdx, m_configInfo.loops, m_configInfo.fps, m_configInfo.prefix);
                stbi_image_free(data);
            }
            m_pngIdx++;
            return 0;
        });
    } while (false);
    return i_err;
}

int HJImgSeqParse::update()
{
    int i_err = HJ_OK;
    do
    {
        i_err = priCreateTexture();
        if (i_err < 0)
        {
            break;
        }   
        HJRawImageDataInfo::Ptr rawInfo = m_cache.acquire();
        if (!rawInfo)
        {
            i_err = HJ_WOULD_BLOCK;
            break;
        }    
        m_width = rawInfo->m_width;
        m_height = rawInfo->m_height;

        if (m_idxNotify)
        {
            m_idxNotify(rawInfo->m_imgIdx);
        }

        i_err = priUpdate(rawInfo);
        if (i_err < 0)
        {
            break;
        }    

        m_cache.recovery(rawInfo);
    } while (false);
    return i_err;
}

int HJImgSeqParse::getWidth()
{
    return m_width;
}
int HJImgSeqParse::getHeight()
{
    return m_height;
}
GLuint HJImgSeqParse::getTextureId()
{
    return m_texture;
}
bool HJImgSeqParse::IsStateReady()
{
    return m_bTextureReady;
}

int HJImgSeqParse::priUpdate(HJRawImageDataInfo::Ptr i_rawInfo)
{
    int i_err = 0;
    do 
    {
        HJOGCommon::textureUpload(m_texture, i_rawInfo->m_width, i_rawInfo->m_height, i_rawInfo->m_components, i_rawInfo->m_buffer->getBuf());
    } while (false);
    return i_err;
}

int HJImgSeqParse::priCreateTexture()
{
    int i_err = 0;
    do 
    {
        if (m_bTextureReady)
        {
            break;
        } 
        m_texture = HJOGCommon::textureCreate();
        m_bTextureReady = true;

	} while (false);
	return i_err;
}
NS_HJ_END