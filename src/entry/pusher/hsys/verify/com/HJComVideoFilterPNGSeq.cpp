#include "HJComVideoFilterPNGSeq.h"
#include "HJFLog.h"
#include "HJBaseUtils.h"
#include "HJComEvent.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

NS_HJ_BEGIN

HJComVideoFilterPNGSeq::HJComVideoFilterPNGSeq()
{
    HJ_SetInsName(HJComVideoFilterPNGSeq);    
    setFilterType(HJCOM_FILTER_TYPE_VIDEO);
}
HJComVideoFilterPNGSeq::~HJComVideoFilterPNGSeq()
{
    HJFLogi("~HJComVideoFilterPNGSeq");
}
int HJComVideoFilterPNGSeq::init(HJBaseParam::Ptr i_param)
{
    int i_err = 0;
    do 
    {
        HJFLogi("{} init enter", m_insName);
        i_err = HJBaseRenderCom::init(i_param);
        if (i_err < 0)
        {
            break;
        }
        std::string path = "";
        if (i_param->contains("pngsequrl"))
        {
            path = i_param->getValue<std::string>("pngsequrl");
        }
        if (path.empty())
        {
            i_err = -1;
            break;
        }
        
        std::string configUrl = HJBaseUtils::combineUrl(path, "config.json");
        if (configUrl.empty())
        {
            i_err = -1;
            break;
        }    
        std::string config = HJBaseUtils::readFileToString(configUrl);
        if (config.empty())
        {
            i_err = -1;
            break;
        }    
        
        i_err = m_configInfo.init(config);
        if (i_err < 0)
        {
            break;
        }
        
        HJFLogi("{} config enter loop:{} fps:{} prefix:{}", m_insName, m_configInfo.loops, m_configInfo.fps, m_configInfo.prefix);
                
        std::vector<std::string> urls = HJBaseUtils::getSortedFiles(path, m_configInfo.prefix);
        if (!urls.empty())
        {
            m_pngUrlQueue = std::move(urls);   
            m_pngIdx = 0;
        }

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
                        m_notify(HJVIDEOCAPTURE_EVENT_PNGSEQ_COMPLETE, HJBaseNotifyInfo{});
                    }    
                    HJFLogi("end, not proc");
                    return 0;
                }    
            }
            
            std::string url = m_pngUrlQueue[m_pngIdx++];
            int width = 0, height = 0, nrComponents = 0;
		    unsigned char *data = stbi_load(url.c_str(), &width, &height, &nrComponents, 0);
            if (data)
            {
                HJRawImageDataInfo::Ptr rawInfo = HJRawImageDataInfo::Create();
                rawInfo->m_width = width;
                rawInfo->m_height = height;
                rawInfo->m_components = nrComponents;
                rawInfo->m_buffer = HJSPBuffer::create(width * height * nrComponents, data);
                m_cache.enqueue(rawInfo);
                //HJFLogi("{} decode png end idx:{} loop:{} fps:{} prefix:{}", m_insName, m_pngIdx, m_configInfo.getloops(), m_configInfo.getfps(), m_configInfo.getprefix());
                stbi_image_free(data);
            }
            return 0;
        });
    } while (false);
    return i_err;
}

void HJComVideoFilterPNGSeq::done() 
{
    HJFLogi("{} doRenderDone", m_insName);   
    if (m_threadTimer)
    {
        m_threadTimer->stopTimer();
        m_threadTimer = nullptr;
    }    
    
    if (m_bTextureReady)
    {
        glDeleteTextures(1, &m_texture);
        m_bTextureReady = false;
    }
    if (m_draw)
    {
        m_draw->release();
        m_draw = nullptr;
    } 
    
    HJBaseRenderCom::done();
}

int HJComVideoFilterPNGSeq::doRenderUpdate() 
{
    int i_err = 0;
    do 
    {
        //HJFLogi("{} doRenderUpdate enter loop:{} fps:{} prefix:{}", m_insName, m_configInfo.getloops(), m_configInfo.getfps(), m_configInfo.getprefix());
        i_err = priCreateTexture();
        if (i_err < 0)
        {
            break;
        }
        i_err = renderUpdate();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;    
}

int HJComVideoFilterPNGSeq::priUpdate(HJRawImageDataInfo::Ptr i_rawInfo)
{
    int i_err = 0;
    do 
    {
		GLenum internalFormat;
		GLenum dataFormat;
		if (i_rawInfo->m_components == 3)
		{
			internalFormat = GL_RGB;
			dataFormat = GL_RGB;
		}
		else if (i_rawInfo->m_components == 4)
		{
			internalFormat = GL_RGBA;
			dataFormat = GL_RGBA;
		}
        
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, i_rawInfo->m_width, i_rawInfo->m_height, 0, dataFormat, GL_UNSIGNED_BYTE, i_rawInfo->m_buffer->getBuf());
		glBindTexture(GL_TEXTURE_2D, 0);
	
    } while (false);
    return i_err;
}

int HJComVideoFilterPNGSeq::priCreateTexture()
{
    int i_err = 0;
    do 
    {
        if (m_bTextureReady)
        {
            break;
        }    
        glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
                
        m_bTextureReady = true;

	} while (false);
	return i_err;
}

int HJComVideoFilterPNGSeq::doRenderDraw(int i_targetWidth, int i_targetHeight) 
{
    int i_err = 0;
    do 
    {
        //HJFLogi("{} doRenderDraw enter loop:{} fps:{} prefix:{}", m_insName, m_configInfo.getloops(), m_configInfo.getfps(), m_configInfo.getprefix());
        
        HJRawImageDataInfo::Ptr rawInfo = m_cache.acquire();
        if (!rawInfo)
        {
            break;
        }    
        i_err = priUpdate(rawInfo);
        if (i_err < 0)
        {
            break;
        }    
        if (!m_draw)
		{
			m_draw = HJOGCopyShaderStrip::Create();
			i_err = m_draw->init(OGCopyShaderStripFlag_2D);
			HJFLogi("Draw init i_err:{}", i_err);
		}
        int x = i_targetWidth * m_configInfo.position.topx;
        int y = (1.f - m_configInfo.position.height - m_configInfo.position.topy) * i_targetHeight;
        int w = i_targetWidth * m_configInfo.position.width;
        int h = i_targetHeight * m_configInfo.position.height;
        glViewport(x, y, w, h);
        
		i_err = m_draw->draw(m_texture, "clip", rawInfo->m_width, rawInfo->m_height, w, h, m_matrix, true);
        if (i_err < 0)
        {
            break;
        }
        
        i_err = renderDraw(i_targetWidth, i_targetHeight);
        if (i_err < 0)
        {
            break;
        }
        
        bool bRecory = m_cache.recovery(rawInfo);
        //HJFLogi("{} bRecory:{}", m_insName, bRecory);
    } while (false);
    return i_err;    
}

NS_HJ_END