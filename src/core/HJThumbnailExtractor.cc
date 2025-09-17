//***********************************************************************************//
//HJMediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJThumbnailExtractor.h"
#include "HJFileUtil.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJThumbnailExtractor::HJThumbnailExtractor()
{
    
}

HJThumbnailExtractor::~HJThumbnailExtractor()
{
    HJLogi("done entry");
    m_bQuit = true;
    if (m_excutor) {
        m_excutor->stop();
        m_excutor = nullptr;
    }
    HJLogi("done end");
}

int HJThumbnailExtractor::init(const std::string& url, const HJThumbnailInfo::Ptr& thumbInfo)
{
    if(url.empty()){
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    HJLogi("init entry, url:" + url);
    do {
//        m_env = std::make_shared<HJEnvironment>();
//        if(!m_env) {
//            res = HJErrNewObj;
//            break;
//        }
        m_mediaUrl = std::make_shared<HJMediaUrl>(url);
        m_mediaUrl->setDisableMFlag(HJMEDIA_TYPE_AUDIO | HJMEDIA_TYPE_SUBTITLE);
        m_thumbInfo = thumbInfo;
        //
        m_source = std::make_shared<HJFFDemuxer>();
        res = m_source->init(m_mediaUrl);
        if(HJ_OK != res){
            HJLoge("error, source init failed, res:" + HJ2STR(res));
            break;
        }
        m_minfo = m_source->getMediaInfo();
        if(!m_minfo) {
            res = HJErrNotMInfo;
            HJLoge("error, get media info failed");
            break;
        }
        HJVideoInfo::Ptr videoInfo = m_minfo->getVideoInfo();
        if(!videoInfo) {
            res = HJErrNotMInfo;
            HJLoge("error, get video info failed");
            break;
        }
        (*videoInfo)["threads"] = (int)1;
        //
        if(m_thumbInfo->m_seekStep <= 0) {
            m_thumbInfo->m_seekStep = (m_thumbInfo->m_count < 2) ? (m_minfo->getDuration() + HJ_THUM_STEP_MIN) : (m_minfo->getDuration() / (m_thumbInfo->m_count - 1));
        }
        m_thumbInfo->m_seekStep = HJ_CLIP(m_thumbInfo->m_seekStep, HJ_THUM_STEP_MIN, m_minfo->getDuration());
        //
        m_decoder = HJBaseCodec::createVDecoder();
        res = m_decoder->init(videoInfo);
        if(HJ_OK != res){
            HJLoge("error, decoder init failed, res:" + HJ2STR(res));
            break;
        }
        m_excutor = HJExecutorManager::createExecutor(HJ_TYPE_NAME(HJThumbnailExtractor));
        if(!m_excutor) {
            res = HJErrNewObj;
            break;
        }
        m_imageWriter = std::make_shared<HJImageWriter>();
        if(m_thumbInfo->m_format == "png") {
            res = m_imageWriter->initWithPNG(m_thumbInfo->m_width, m_thumbInfo->m_height);
        } else {
            res = m_imageWriter->initWithJPG(m_thumbInfo->m_width, m_thumbInfo->m_height, m_thumbInfo->m_quality);
        }
        if(HJ_OK != res){
            HJLoge("error, image writer init failed, res:" + HJ2STR(res));
            break;
        }
        notify(HJThumbnailNotify_Init);
    } while (false);
    
    return res;
}

int HJThumbnailExtractor::run()
{
    if(!m_source || !m_imageWriter) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    int64_t duration = m_minfo->getDuration();
    //
    while((m_seekPos <= duration) && !m_bQuit)
    {
        HJLogi("run get thumb pos:" + HJ2STR(m_seekPos));
        res = m_source->seek(m_seekPos);
        if (HJ_OK != res) {
            HJLoge("source seek time:" + HJ2String(m_seekPos) + " error:" + HJ2STR(res));
            break;
        }
        HJMediaFrame::Ptr outFrame = nullptr;
        res = decodeVideoFrame(outFrame);
        if(HJ_EOF == res || res < HJ_OK) {
            HJLoge("error, decode video frame failed, res:" + HJ2STR(res));
            break;
        }
        if(outFrame)
        {
            HJLogi("run get decode video frame:" + outFrame->formatInfo());
            std::string workDir = m_thumbInfo->m_workDir + "/";// + HJ2STR(m_mediaUrl->getUrlHash()) + "/";
            HJFileUtil::makeDir(workDir.c_str());
            //
            std::string outUrl = workDir + HJ2STR(m_seekPos);
            if(m_thumbInfo->m_format == "png") {
                outUrl += + ".png";
            } else {
                outUrl += + ".jpg";
            }
            res = m_imageWriter->writeFrame(outFrame, outUrl);
            if(HJ_OK != res) {
                HJLoge("error, image writer frame failed, res:" + HJ2STR(res));
                break;
            }
            if(HJFileUtil::fileExist(outUrl.c_str())) {
                notify(HJThumbnailNotify_Url, outUrl);
            }
        } else {
            HJLogi("warning, mavf is null");
        }
        m_seekPos += m_thumbInfo->m_seekStep;
    }
    if(res < HJ_OK) {
        notify(HJThumbnailNotify_Error, HJ2STR(res));
    }
    notify(HJThumbnailNotify_Completed);
    
    HJLogi("run end");
    
    return res;
}

int HJThumbnailExtractor::asyncRun()
{
    if(!m_source || !m_imageWriter || !m_excutor) {
        return HJErrNotAlready;
    }
    HJThumbnailExtractor::Wtr wtr = sharedFrom(this);
    m_excutor->async([wtr](){
        auto ptr = wtr.lock();
        if(!ptr) {
            return;
        }
        ptr->run();
    });
    
    return HJ_OK;
}


int HJThumbnailExtractor::decodeVideoFrame(HJMediaFrame::Ptr& outFrame)
{
    int res = HJ_OK;
    for(;;)
    {
        HJMediaFrame::Ptr smavf =  nullptr;
        res = m_source->getFrame(smavf);
        if(HJ_EOF == res || res < HJ_OK) {
            HJLoge("error, source get frame failed");
            break;
        }
        HJLogi("source get frame:" + smavf->formatInfo());
        if(smavf && (HJMEDIA_TYPE_VIDEO == smavf->getType()))
        {
            res = m_decoder->run(smavf);
            if(HJ_EOF == res || res < HJ_OK) {
                HJLoge("error, decoder run frame failed");
                break;
            }
            res = m_decoder->getFrame(outFrame);
            if(HJ_EOF == res || res < HJ_OK) {
                HJLoge("error, decoder run frame failed");
                break;
            }
            if(outFrame) {
                HJLogi("aready get decode frame");
                break;
            }
        }
    }
    m_decoder->flush();
    
    return res;
}

int HJThumbnailExtractor::notify(const size_t identify, const std::string& msg)
{
    HJNotification::Ptr ntf = HJMakeNotification(identify, msg);
    return HJEnvironment::notify(ntf);
}

//***********************************************************************************//
HJThumbnailInfo::HJThumbnailInfo(const std::string& workDir, int width, int height, int count, int64_t step, std::string format, float quality)
    : m_workDir(workDir)
    , m_width(width)
    , m_height(height)
    , m_count(count)
    , m_seekStep(step)
    , m_format(format)
    , m_quality(quality)
{
    
}

HJThumbnailInfo::~HJThumbnailInfo()
{
    
}

NS_HJ_END
