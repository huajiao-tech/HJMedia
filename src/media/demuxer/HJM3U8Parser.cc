//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJM3U8Parser.h"
#include "HJFFUtils.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJXIOFile.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

NS_HJ_BEGIN
//***********************************************************************************//

// 静态成员初始化
const std::vector<HJM3U8SegmentGroup::Ptr> HJM3U8Parser::m_emptyGroups;
const std::vector<HJM3U8Variant::Ptr> HJM3U8Parser::m_emptyVariants;

//***********************************************************************************//
// 析构函数
//***********************************************************************************//
HJM3U8Parser::~HJM3U8Parser()
{
    reset();
}

//***********************************************************************************//
// 重置解析器状态
//***********************************************************************************//
void HJM3U8Parser::reset()
{
    if (m_xio) {
        m_xio->close();
        m_xio = nullptr;
    }
    m_playlist = nullptr;
    m_mediaUrl = nullptr;
    m_progressCallback = nullptr;
}

//***********************************************************************************//
// 从URL初始化
//***********************************************************************************//
int HJM3U8Parser::init(const HJMediaUrl::Ptr& mediaUrl)
{
    int res = HJ_OK;
    
    do {
        if (!mediaUrl) {
            HJLoge("error, mediaUrl is null");
            return HJErrInvalidParams;
        }
        
        m_mediaUrl = mediaUrl;
        std::string url = m_mediaUrl->getUrl();
        
        if (url.empty()) {
            HJLoge("error, input url is empty");
            return HJErrInvalidUrl;
        }
        
        // 创建IO上下文并打开URL
        HJUrl::Ptr murl = std::make_shared<HJUrl>(url, HJ_XIO_READ);
        (*murl)["multiple_requests"] = static_cast<int>(1);
        
        m_xio = std::make_shared<HJXIOContext>();
        res = m_xio->open(murl);
        if (HJ_OK != res) {
            HJLoge("error, failed to open url: " + url);
            break;
        }
        
        // 读取文件内容
        int64_t fileSize = m_xio->size();
        if (fileSize <= 0) {
            HJLoge("error, file size is invalid");
            res = HJErrInvalidData;
            break;
        }
        
        // 限制文件大小 (M3U8通常不应超过10MB)
        const int64_t MAX_M3U8_SIZE = 10 * 1024 * 1024;
        if (fileSize > MAX_M3U8_SIZE) {
            HJLogw("warning, M3U8 file too large, truncating to 10MB");
            fileSize = MAX_M3U8_SIZE;
        }
        
        std::string content;
        content.reserve(static_cast<size_t>(fileSize));
        
        char line[MAX_URL_SIZE];
        while (!m_xio->eof()) {
            int len = m_xio->getLine(line, sizeof(line));
            if (len <= 0) {
                break;
            }
            content.append(line, len);
        }
        
        // 解析内容
        res = parseContent(content, url);
        
    } while (false);
    
    return res;
}

//***********************************************************************************//
// 从字符串内容初始化
//***********************************************************************************//
int HJM3U8Parser::initFromContent(const std::string& content, const std::string& baseUrl)
{
    if (content.empty()) {
        HJLoge("error, content is empty");
        return HJErrInvalidParams;
    }
    
    return parseContent(content, baseUrl);
}

//***********************************************************************************//
// 解析M3U8内容
//***********************************************************************************//
int HJM3U8Parser::parseContent(const std::string& content, const std::string& baseUrl)
{
    // 分割成行
    std::vector<std::string> lines = splitLines(content);
    
    if (lines.empty()) {
        HJLoge("error, no lines in content");
        return HJErrInvalidData;
    }
    
    // 检查M3U8文件头
    bool hasExtM3U = false;
    for (const auto& line : lines) {
        if (line.find("#EXTM3U") != std::string::npos) {
            hasExtM3U = true;
            break;
        }
    }
    
    if (!hasExtM3U) {
        HJLoge("error, not a valid M3U8 file (missing #EXTM3U)");
        return HJErrInvalidData;
    }
    
    // 创建播放列表对象
    m_playlist = std::make_shared<HJM3U8Playlist>();
    
    // 判断是Master Playlist还是Media Playlist
    // Master Playlist 包含 EXT-X-STREAM-INF 或 EXT-X-I-FRAME-STREAM-INF
    bool isMaster = false;
    for (const auto& line : lines) {
        if (line.find("#EXT-X-STREAM-INF") != std::string::npos ||
            line.find("#EXT-X-I-FRAME-STREAM-INF") != std::string::npos) {
            isMaster = true;
            break;
        }
    }
    
    m_playlist->isMasterPlaylist = isMaster;
    
    int res;
    if (isMaster) {
        res = parseMasterPlaylist(lines, baseUrl);
    } else {
        res = parseMediaPlaylist(lines, baseUrl);
    }
    
    return res;
}

//***********************************************************************************//
// 解析Master Playlist
//***********************************************************************************//
int HJM3U8Parser::parseMasterPlaylist(const std::vector<std::string>& lines, const std::string& baseUrl)
{
    HJM3U8Variant::Ptr pendingVariant;
    int totalVariants = 0;
    int parsedVariants = 0;
    
    // 预先统计变体数量
    for (const auto& line : lines) {
        if (line.find("#EXT-X-STREAM-INF") != std::string::npos) {
            totalVariants++;
        }
    }
    
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        
        if (line.empty() || line[0] == '\r' || line[0] == '\n') {
            continue;
        }
        
        // 解析版本
        if (line.find("#EXT-X-VERSION:") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                m_playlist->version = std::atoi(line.c_str() + pos + 1);
            }
        }
        // 解析流变体信息
        else if (line.find("#EXT-X-STREAM-INF:") != std::string::npos) {
            pendingVariant = std::make_shared<HJM3U8Variant>();
            parseStreamInf(line, *pendingVariant);
        }
        // 非注释行且有待处理的变体 - 这是变体的URI
        else if (!line.empty() && line[0] != '#' && pendingVariant) {
            pendingVariant->uri = makeAbsoluteUrl(baseUrl, line);
            m_playlist->variants.push_back(pendingVariant);
            pendingVariant = nullptr;

            parsedVariants++;
            if (m_progressCallback) {
                m_progressCallback(parsedVariants, totalVariants);
            }
        }
    }
    
    HJFLogi("Parsed Master Playlist: {} variants", m_playlist->variants.size());
    
    // 自动解析所有变体的子播放列表
    if (!m_playlist->variants.empty()) {
        return parseAllVariants(true);
    }
    
    return HJ_OK;
}

//***********************************************************************************//
// 解析Media Playlist
//***********************************************************************************//
int HJM3U8Parser::parseMediaPlaylist(const std::vector<std::string>& lines, const std::string& baseUrl)
{
    HJM3U8Key currentKey;
    HJM3U8Segment pendingSegment;
    bool hasPendingSegment = false;
    bool hasDiscontinuity = false;
    int64_t currentByteRangeStart = 0;
    int64_t playlistTimeMs = 0;
    int totalSegmentCount = 0;  // 本地分片计数器
    
    // 当前分片组
    auto currentGroup = std::make_shared<HJM3U8SegmentGroup>();
    currentGroup->groupIndex = 0;
    currentGroup->discontinuitySequence = m_playlist->discontinuitySequence;
    currentGroup->startTimeMs = 0;
    
    int totalLines = static_cast<int>(lines.size());
    int parsedLines = 0;
    
    for (const auto& line : lines) {
        parsedLines++;
        
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        
        // 去除行尾的\r\n
        std::string trimmedLine = line;
        while (!trimmedLine.empty() && 
               (trimmedLine.back() == '\r' || trimmedLine.back() == '\n')) {
            trimmedLine.pop_back();
        }
        
        if (trimmedLine.empty()) {
            continue;
        }
        
        // #EXT-X-VERSION
        if (trimmedLine.find("#EXT-X-VERSION:") != std::string::npos) {
            size_t pos = trimmedLine.find(':');
            if (pos != std::string::npos) {
                m_playlist->version = std::atoi(trimmedLine.c_str() + pos + 1);
            }
        }
        // #EXT-X-TARGETDURATION
        else if (trimmedLine.find("#EXT-X-TARGETDURATION:") != std::string::npos) {
            size_t pos = trimmedLine.find(':');
            if (pos != std::string::npos) {
                m_playlist->targetDuration = std::atoi(trimmedLine.c_str() + pos + 1);
            }
        }
        // #EXT-X-MEDIA-SEQUENCE
        else if (trimmedLine.find("#EXT-X-MEDIA-SEQUENCE:") != std::string::npos) {
            size_t pos = trimmedLine.find(':');
            if (pos != std::string::npos) {
                m_playlist->mediaSequence = std::atoi(trimmedLine.c_str() + pos + 1);
            }
        }
        // #EXT-X-DISCONTINUITY-SEQUENCE
        else if (trimmedLine.find("#EXT-X-DISCONTINUITY-SEQUENCE:") != std::string::npos) {
            size_t pos = trimmedLine.find(':');
            if (pos != std::string::npos) {
                m_playlist->discontinuitySequence = std::atoi(trimmedLine.c_str() + pos + 1);
                currentGroup->discontinuitySequence = m_playlist->discontinuitySequence;
            }
        }
        // #EXT-X-PLAYLIST-TYPE
        else if (trimmedLine.find("#EXT-X-PLAYLIST-TYPE:") != std::string::npos) {
            if (trimmedLine.find("VOD") != std::string::npos) {
                m_playlist->isVOD = true;
            } else if (trimmedLine.find("EVENT") != std::string::npos) {
                m_playlist->isEvent = true;
            }
        }
        // #EXT-X-DISCONTINUITY
        else if (trimmedLine.find("#EXT-X-DISCONTINUITY") != std::string::npos &&
                 trimmedLine.find("#EXT-X-DISCONTINUITY-SEQUENCE") == std::string::npos) {
            // 保存当前组 (如果有分片)
            if (!currentGroup->empty()) {
                m_playlist->segmentGroups.push_back(currentGroup);
                
                // 创建新组
                currentGroup = std::make_shared<HJM3U8SegmentGroup>();
                currentGroup->groupIndex = static_cast<int>(m_playlist->segmentGroups.size());
                currentGroup->discontinuitySequence = m_playlist->discontinuitySequence + 
                                                       static_cast<int>(m_playlist->segmentGroups.size());
                currentGroup->startTimeMs = playlistTimeMs;
            }
            hasDiscontinuity = true;
        }
        // #EXT-X-KEY
        else if (trimmedLine.find("#EXT-X-KEY:") != std::string::npos) {
            parseKey(trimmedLine, currentKey, baseUrl);
        }
        // #EXT-X-BYTERANGE
        else if (trimmedLine.find("#EXT-X-BYTERANGE:") != std::string::npos) {
            int64_t length = 0, start = 0;
            if (parseByteRange(trimmedLine, start, length)) {
                if (start < 0) {
                    // 无起始位置,使用上次的结束位置
                    start = currentByteRangeStart;
                }
                pendingSegment.byteRangeStart = start;
                pendingSegment.byteRangeLength = length;
                currentByteRangeStart = start + length;
            }
        }
        // #EXTINF
        else if (trimmedLine.find("#EXTINF:") != std::string::npos) {
            double duration = 0.0;
            std::string title;
            if (parseExtInf(trimmedLine, duration, title)) {
                pendingSegment.duration = duration;
                pendingSegment.title = title;
                pendingSegment.key = currentKey;
                pendingSegment.hasDiscontinuityBefore = hasDiscontinuity;
                hasDiscontinuity = false;
                hasPendingSegment = true;
            }
        }
        // #EXT-X-ENDLIST
        else if (trimmedLine.find("#EXT-X-ENDLIST") != std::string::npos) {
            m_playlist->isVOD = true;
        }
        // 非注释行 - 分片URI
        else if (!trimmedLine.empty() && trimmedLine[0] != '#' && hasPendingSegment) {
            pendingSegment.uri = makeAbsoluteUrl(baseUrl, trimmedLine);
            pendingSegment.startTimeMs = playlistTimeMs;
            pendingSegment.mediaSequence = m_playlist->mediaSequence + totalSegmentCount;
            
            // 添加到当前组
            currentGroup->addSegment(pendingSegment);
            totalSegmentCount++;  // 增加本地计数器
            
            // 更新播放列表时间线
            playlistTimeMs += static_cast<int64_t>(pendingSegment.duration * 1000);
            
            // 重置待处理分片
            pendingSegment = HJM3U8Segment();
            hasPendingSegment = false;
            
            // 进度回调
            if (m_progressCallback && parsedLines % 10 == 0) {
                m_progressCallback(parsedLines, totalLines);
            }
        }
    }
    
    // 保存最后一个组
    if (!currentGroup->empty()) {
        m_playlist->segmentGroups.push_back(currentGroup);
    }
    
    // 计算总时长
    m_playlist->totalDurationMs = playlistTimeMs;
    
    HJFLogi("Parsed Media Playlist: {} groups, {} segments, duration={}ms",
            m_playlist->getGroupCount(),
            m_playlist->getTotalSegmentCount(),
            m_playlist->totalDurationMs);
    
    return HJ_OK;
}

//***********************************************************************************//
// 解析 #EXTINF 标签
//***********************************************************************************//
bool HJM3U8Parser::parseExtInf(const std::string& line, double& duration, std::string& title)
{
    // 格式: #EXTINF:<duration>[,<title>]
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    std::string value = line.substr(colonPos + 1);
    
    // 查找逗号分隔符
    size_t commaPos = value.find(',');
    std::string durationStr;
    
    if (commaPos != std::string::npos) {
        durationStr = value.substr(0, commaPos);
        title = value.substr(commaPos + 1);
        // 去除title的前后空格
        while (!title.empty() && std::isspace(title.front())) title.erase(0, 1);
        while (!title.empty() && std::isspace(title.back())) title.pop_back();
    } else {
        durationStr = value;
    }
    
    // 去除duration字符串的前后空格和可能的换行符
    while (!durationStr.empty() && (std::isspace(durationStr.back()) || 
           durationStr.back() == '\r' || durationStr.back() == '\n')) {
        durationStr.pop_back();
    }
    
    duration = std::atof(durationStr.c_str());
    return duration >= 0;
}

//***********************************************************************************//
// 解析 #EXT-X-STREAM-INF 标签
//***********************************************************************************//
bool HJM3U8Parser::parseStreamInf(const std::string& line, HJM3U8Variant& variant)
{
    // 解析 BANDWIDTH
    std::string bandwidth = getAttribute(line, "BANDWIDTH");
    if (!bandwidth.empty()) {
        variant.bandwidth = std::atoi(bandwidth.c_str());
    }
    
    // 解析 AVERAGE-BANDWIDTH
    std::string avgBandwidth = getAttribute(line, "AVERAGE-BANDWIDTH");
    if (!avgBandwidth.empty()) {
        variant.averageBandwidth = std::atoi(avgBandwidth.c_str());
    }
    
    // 解析 RESOLUTION
    std::string resolution = getAttribute(line, "RESOLUTION");
    if (!resolution.empty()) {
        variant.resolution = resolution;
        parseResolution(resolution, variant.width, variant.height);
    }
    
    // 解析 CODECS
    variant.codecs = getAttribute(line, "CODECS");
    
    // 解析 FRAME-RATE
    std::string frameRate = getAttribute(line, "FRAME-RATE");
    if (!frameRate.empty()) {
        variant.frameRate = std::atof(frameRate.c_str());
    }
    
    // 解析 AUDIO
    variant.audio = getAttribute(line, "AUDIO");
    
    // 解析 VIDEO
    variant.video = getAttribute(line, "VIDEO");
    
    // 解析 SUBTITLES
    variant.subtitles = getAttribute(line, "SUBTITLES");
    
    return true;
}

//***********************************************************************************//
// 解析 #EXT-X-KEY 标签
//***********************************************************************************//
bool HJM3U8Parser::parseKey(const std::string& line, HJM3U8Key& key, const std::string& baseUrl)
{
    // 解析 METHOD
    std::string method = getAttribute(line, "METHOD");
    if (method == "NONE") {
        key.method = HJM3U8EncryptMethod::None;
    } else if (method == "AES-128") {
        key.method = HJM3U8EncryptMethod::AES128;
    } else if (method == "SAMPLE-AES") {
        key.method = HJM3U8EncryptMethod::SampleAES;
    }
    
    // 解析 URI
    std::string uri = getAttribute(line, "URI");
    if (!uri.empty()) {
        key.uri = makeAbsoluteUrl(baseUrl, uri);
    }
    
    // 解析 IV
    key.iv = getAttribute(line, "IV");
    
    return true;
}

//***********************************************************************************//
// 解析 #EXT-X-BYTERANGE 标签
//***********************************************************************************//
bool HJM3U8Parser::parseByteRange(const std::string& line, int64_t& start, int64_t& length)
{
    // 格式: #EXT-X-BYTERANGE:<n>[@<o>]
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    std::string value = line.substr(colonPos + 1);
    
    size_t atPos = value.find('@');
    if (atPos != std::string::npos) {
        length = std::atoll(value.substr(0, atPos).c_str());
        start = std::atoll(value.substr(atPos + 1).c_str());
    } else {
        length = std::atoll(value.c_str());
        start = -1; // 表示使用上次的结束位置
    }
    
    return true;
}

//***********************************************************************************//
// 生成绝对URL
//***********************************************************************************//
std::string HJM3U8Parser::makeAbsoluteUrl(const std::string& baseUrl, const std::string& relativeUrl)
{
    if (relativeUrl.empty()) {
        return relativeUrl;
    }
    
    // 去除引号
    std::string url = relativeUrl;
    if (url.front() == '"' && url.back() == '"') {
        url = url.substr(1, url.size() - 2);
    }
    
    // 去除尾部空白
    while (!url.empty() && (std::isspace(url.back()) || 
           url.back() == '\r' || url.back() == '\n')) {
        url.pop_back();
    }
    
    // 已经是绝对URL
    if (url.find("http://") == 0 || url.find("https://") == 0 || url.find("file://") == 0) {
        return url;
    }
    
    // 使用FFmpeg工具转换
    char absoluteUrl[MAX_URL_SIZE];
    ff_make_absolute_url(absoluteUrl, sizeof(absoluteUrl), baseUrl.c_str(), url.c_str());
    
    if (absoluteUrl[0]) {
        return std::string(absoluteUrl);
    }
    
    return url;
}

//***********************************************************************************//
// 分割内容为行
//***********************************************************************************//
std::vector<std::string> HJM3U8Parser::splitLines(const std::string& content)
{
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // 保留行内容,包括可能的\r
        lines.push_back(line);
    }
    
    return lines;
}

//***********************************************************************************//
// 获取属性值
//***********************************************************************************//
std::string HJM3U8Parser::getAttribute(const std::string& line, const std::string& key)
{
    // 查找 KEY= 或 KEY="
    std::string searchKey = key + "=";
    size_t pos = line.find(searchKey);
    
    if (pos == std::string::npos) {
        return "";
    }
    
    pos += searchKey.length();
    
    // 处理带引号的值
    if (pos < line.length() && line[pos] == '"') {
        pos++;
        size_t endPos = line.find('"', pos);
        if (endPos != std::string::npos) {
            return line.substr(pos, endPos - pos);
        }
    }
    
    // 不带引号的值,以逗号或行尾结束
    size_t endPos = line.find(',', pos);
    if (endPos == std::string::npos) {
        endPos = line.length();
    }
    
    std::string value = line.substr(pos, endPos - pos);
    
    // 去除尾部空白
    while (!value.empty() && (std::isspace(value.back()) || 
           value.back() == '\r' || value.back() == '\n')) {
        value.pop_back();
    }
    
    return value;
}

//***********************************************************************************//
// 解析分辨率
//***********************************************************************************//
int HJM3U8Parser::parseResolution(const std::string& resolution, int& width, int& height)
{
    // 格式: WIDTHxHEIGHT
    size_t xPos = resolution.find('x');
    if (xPos == std::string::npos) {
        xPos = resolution.find('X');
    }
    
    if (xPos == std::string::npos) {
        return HJErrInvalidData;
    }
    
    width = std::atoi(resolution.substr(0, xPos).c_str());
    height = std::atoi(resolution.substr(xPos + 1).c_str());
    
    return HJ_OK;
}

//***********************************************************************************//
// 根据时间查找分片
//***********************************************************************************//
bool HJM3U8Parser::findSegmentByTime(int64_t timeMs, int& outGroupIndex, int& outSegmentIndex) const
{
    if (!m_playlist || m_playlist->segmentGroups.empty()) {
        return false;
    }
    
    int64_t currentTimeMs = 0;
    
    for (size_t gi = 0; gi < m_playlist->segmentGroups.size(); ++gi) {
        const auto& group = m_playlist->segmentGroups[gi];
        
        for (size_t si = 0; si < group->segments.size(); ++si) {
            const auto& segment = group->segments[si];
            int64_t segmentDurationMs = static_cast<int64_t>(segment.duration * 1000);
            
            if (timeMs >= currentTimeMs && timeMs < currentTimeMs + segmentDurationMs) {
                outGroupIndex = static_cast<int>(gi);
                outSegmentIndex = static_cast<int>(si);
                return true;
            }
            
            currentTimeMs += segmentDurationMs;
        }
    }
    
    return false;
}

//***********************************************************************************//
// 根据时间获取分片
//***********************************************************************************//
const HJM3U8Segment* HJM3U8Parser::getSegmentByTime(int64_t timeMs) const
{
    int groupIndex = 0;
    int segmentIndex = 0;
    
    if (findSegmentByTime(timeMs, groupIndex, segmentIndex)) {
        const auto& groups = m_playlist->segmentGroups;
        if (groupIndex >= 0 && groupIndex < static_cast<int>(groups.size())) {
            const auto& group = groups[groupIndex];
            if (segmentIndex >= 0 && segmentIndex < static_cast<int>(group->segments.size())) {
                return &group->segments[segmentIndex];
            }
        }
    }
    
    return nullptr;
}

//***********************************************************************************//
// 解析所有变体的子播放列表
//***********************************************************************************//
int HJM3U8Parser::parseAllVariants(bool parseAll)
{
    if (!m_playlist || !m_playlist->isMasterPlaylist) {
        HJLoge("error, not a master playlist");
        return HJErrInvalid;
    }
    
    if (m_playlist->variants.empty()) {
        HJLogw("warning, no variants in master playlist");
        return HJ_OK;
    }
    
    int successCount = 0;
    int failCount = 0;
    size_t parseCount = parseAll ? m_playlist->variants.size() : 1;
    
    for (size_t i = 0; i < parseCount; ++i) {
        auto& variant = m_playlist->variants[i];
        
        // 跳过已解析的变体
        if (variant->playlist) {
            successCount++;
            continue;
        }
        
        HJFLogi("Parsing variant {}: {} ({}x{}, {}bps)", 
                i, variant->uri, variant->width, variant->height, variant->bandwidth);
        
        // 创建新的解析器来解析子播放列表
        HJMediaUrl::Ptr variantUrl = std::make_shared<HJMediaUrl>(variant->uri);
        HJM3U8Parser subParser;
        
        int res = subParser.init(variantUrl);
        if (res != HJ_OK) {
            HJLogw("warning, failed to parse variant {} playlist: {}", i, res);
            failCount++;
            continue;
        }
        
        variant->playlist = subParser.getPlaylist();
        successCount++;
        
        // 进度回调
        if (m_progressCallback) {
            m_progressCallback(static_cast<int>(i + 1), static_cast<int>(parseCount));
        }
    }
    
    HJFLogi("Parsed {} variants successfully, {} failed", successCount, failCount);
    return (successCount > 0) ? HJ_OK : HJErrFatal;
}

//***********************************************************************************//
// 选择变体
//***********************************************************************************//
int HJM3U8Parser::selectVariant(int index)
{
    if (!m_playlist || !m_playlist->isMasterPlaylist) {
        HJLoge("error, not a master playlist");
        return HJErrInvalid;
    }
    
    if (index < 0 || index >= static_cast<int>(m_playlist->variants.size())) {
        HJLoge("error, variant index out of range");
        return HJErrInvalidParams;
    }
    
    const auto& variant = m_playlist->variants[index];
    
    // 如果变体的子播放列表尚未解析,则解析它
    if (!variant->playlist) {
        // 创建新的解析器来解析子播放列表
        HJMediaUrl::Ptr variantUrl = std::make_shared<HJMediaUrl>(variant->uri);
        HJM3U8Parser subParser;
        
        int res = subParser.init(variantUrl);
        if (res != HJ_OK) {
            HJLoge("error, failed to parse variant playlist");
            return res;
        }
        
        variant->playlist = subParser.getPlaylist();
    }
    
    return HJ_OK;
}

//***********************************************************************************//
// 根据带宽选择最佳变体
//***********************************************************************************//
int HJM3U8Parser::selectVariantByBandwidth(int availableBandwidth) const
{
    if (!m_playlist || !m_playlist->isMasterPlaylist || m_playlist->variants.empty()) {
        return -1;
    }
    
    int bestIndex = 0;
    int bestBandwidth = 0;
    
    for (size_t i = 0; i < m_playlist->variants.size(); ++i) {
        const auto& variant = m_playlist->variants[i];
        
        // 选择不超过可用带宽的最高码率
        if (variant->bandwidth <= availableBandwidth && variant->bandwidth > bestBandwidth) {
            bestBandwidth = variant->bandwidth;
            bestIndex = static_cast<int>(i);
        }
    }
    
    // 如果所有变体都超过可用带宽,选择最低码率
    if (bestBandwidth == 0) {
        int lowestBandwidth = INT_MAX;
        for (size_t i = 0; i < m_playlist->variants.size(); ++i) {
            if (m_playlist->variants[i]->bandwidth < lowestBandwidth) {
                lowestBandwidth = m_playlist->variants[i]->bandwidth;
                bestIndex = static_cast<int>(i);
            }
        }
    }
    
    return bestIndex;
}

NS_HJ_END
