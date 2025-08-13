//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJHLSParser.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJException.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJHLSParser::~HJHLSParser()
{
    m_xio = nullptr;
}

int HJHLSParser::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJ_OK;
    do
    {
        m_mediaUrl = mediaUrl;
        std::string url = m_mediaUrl->getUrl();
        if (url.empty()) {
            HJLoge("error, input url:" + url + " invalid");
            return HJErrInvalidUrl;
        }
        HJUrl::Ptr murl = std::make_shared<HJUrl>(url, HJ_XIO_READ);
        (*murl)["multiple_requests"] = (int)1;
        m_xio = std::make_shared<HJXIOContext>();
        res = m_xio->open(murl);
        if (HJ_OK != res) {
            break;
        }
        int64_t xioLen = m_xio->size();
        HJBuffer::Ptr comUrlBuffer = std::make_shared<HJBuffer>(xioLen);
        //
        char line[MAX_URL_SIZE];
        char tmp_str[MAX_URL_SIZE];
        const char* ptr = NULL;
        HJMediaUrl::Ptr hlsMediaUrl = nullptr;
        HJBuffer::Ptr urlBuffer = comUrlBuffer;
        int segCnt = 0;
        bool is_segment = false;
        int64_t duration = 0;
        //
        while (!m_xio->eof())
        {    
            int len = m_xio->getLine(line, sizeof(line));
            if (len <= 0) {
                HJLogw("warning, get chomp line failed");
                break;
            }
            /**
            * #EXT-X-DISCONTINUITY-SEQUENCE:1
            * #EXT-X-DISCONTINUITY
            */
            if (av_strstart(line, "#EXT-X-DISCONTINUITY", &ptr)) {
                if (segCnt) {
                    const char* endflags = "#EXT-X-ENDLIST\n";
                    urlBuffer->write((const uint8_t*)endflags, strlen(endflags));
                    //
                    m_hlsMediaUrls.emplace_back(std::move(hlsMediaUrl));
                    segCnt = 0;
                } else {
                    HJLogw("warning, invalid discontinuity");
                }
            }
            else if (av_strstart(line, "#EXTINF:", &ptr)) {
                if (!segCnt) {
                    std::string segUrl = HJFMT("{}_{}.m3u8", HJUtilitys::removeSuffix(m_mediaUrl->getUrl()), m_hlsMediaUrls.size());
                    hlsMediaUrl = std::make_shared<HJMediaUrl>(segUrl);
                    //HJFLogi("seg url:{}", segUrl);
                    //
                    urlBuffer = std::make_shared<HJBuffer>(comUrlBuffer);
                    (*hlsMediaUrl)[HJBaseDemuxer::KEY_WORLDS_URLBUFFER] = urlBuffer;
                }
                duration = hlsMediaUrl->getDuration() + atof(ptr) * HJ_MS_PER_SEC/*AV_TIME_BASE*/;
                hlsMediaUrl->setDuration(duration);
                urlBuffer->write((const uint8_t*)line, len);
                segCnt++;
                is_segment = true;
            }
            else if (av_strstart(line, "#EXT-X-ENDLIST", &ptr)) {
                urlBuffer->write((const uint8_t*)line, len);
                //
                m_hlsMediaUrls.emplace_back(std::move(hlsMediaUrl));
                segCnt = 0;
            } else if(line[0]) 
            {
                if (!urlBuffer) {
                    HJLoge("error, hls buffer is null");
                    break;
                }
                urlBuffer->write((const uint8_t*)line, len);
                if (is_segment)
                {
                    ff_make_absolute_url(tmp_str, sizeof(tmp_str), url.c_str(), line);
                    if (!tmp_str[0]) {
                        HJLoge("error, hls url get failed");
                        break;
                    }
                    char* tmpUrl = av_strdup(tmp_str);
                    if (!tmpUrl) {
                        HJLoge("error, hls tmp url get failed");
                        break;
                    }
                    //HJFLogi("absolute url:{}", tmpUrl);
                    HJMediaUrl::Ptr subUrl = std::make_shared<HJMediaUrl>(tmpUrl);
                    subUrl->setDuration(duration);
                    hlsMediaUrl->addSubUrl(subUrl);
                    //
                    is_segment = false;
                }
            } else {
                HJLogw("warning, unkown hls line");
            }
        }
    } while (false);

#if 0
    int flags = AVIO_FLAG_READ;
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "multiple_requests", "1", 0);
    int ret = ffurl_open_whitelist(&m_inner, url.c_str(), flags, m_interruptCB, &opts, NULL, NULL, NULL);
    if (opts) {
        av_dict_free(&opts);
    }
    if (ret < HJ_OK) {
        ffurl_closep(&m_inner);
        return ret;
    }
    int64_t totalSize = ffurl_size(m_inner);
    m_innerBuffer = std::make_shared<HJBuffer>(totalSize);
    while (m_innerBuffer->size() < totalSize)
    {
        ret = ffurl_read(m_inner, m_innerBuffer->spaceData(), m_innerBuffer->space());
        if (AVERROR_EOF != ret) {
            break;
        }
        m_innerBuffer->addSize(ret);
    }
#endif

	return HJ_OK;
}
NS_HJ_END
