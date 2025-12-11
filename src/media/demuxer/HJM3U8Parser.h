//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJBaseDemuxer.h"
#include "HJXIOContext.h"
#include <vector>
#include <memory>
#include <functional>

NS_HJ_BEGIN
//***********************************************************************************//

/**
 * @brief 加密方式枚举
 */
enum class HJM3U8EncryptMethod {
    None = 0,
    AES128,
    SampleAES
};

/**
 * @brief 加密密钥信息
 */
struct HJM3U8Key {
    HJM3U8EncryptMethod method = HJM3U8EncryptMethod::None;
    std::string uri;
    std::string iv;
    
    bool isEncrypted() const {
        return method != HJM3U8EncryptMethod::None;
    }
};

/**
 * @brief 单个分片信息
 */
struct HJM3U8Segment {
    std::string uri;                    // 分片URI (绝对路径)
    double duration = 0.0;              // 分片时长 (秒)
    int64_t startTimeMs = 0;            // 在播放列表中的起始时间 (毫秒)
    int64_t byteRangeStart = -1;        // 字节范围起始 (-1表示无)
    int64_t byteRangeLength = -1;       // 字节范围长度 (-1表示无)
    int mediaSequence = 0;              // 媒体序号
    HJM3U8Key key;                      // 加密信息
    std::string title;                  // 分片标题 (EXTINF后的可选字符串)
    
    bool hasDiscontinuityBefore = false; // 此分片前是否有discontinuity标记
};

/**
 * @brief 分片组 - 按 EXT-X-DISCONTINUITY 分组
 * Composite 模式: 一个组包含多个连续分片
 */
struct HJM3U8SegmentGroup {
    using Ptr = std::shared_ptr<HJM3U8SegmentGroup>;
    
    int groupIndex = 0;                         // 组索引
    int discontinuitySequence = 0;              // discontinuity序号
    std::vector<HJM3U8Segment> segments;        // 分片列表
    int64_t totalDurationMs = 0;                // 组总时长 (毫秒)
    int64_t startTimeMs = 0;                    // 组在播放列表中的起始时间
    
    void addSegment(const HJM3U8Segment& segment) {
        segments.push_back(segment);
        totalDurationMs += static_cast<int64_t>(segment.duration * 1000);
    }
    
    size_t segmentCount() const {
        return segments.size();
    }
    
    bool empty() const {
        return segments.empty();
    }
};

/**
 * @brief 多分辨率变体信息 (Master Playlist中的流)
 */
struct HJM3U8Variant {
    using Ptr = std::shared_ptr<HJM3U8Variant>;
    
    std::string uri;                    // 变体播放列表URI
    int bandwidth = 0;                  // 带宽 (bps)
    int averageBandwidth = 0;           // 平均带宽
    int width = 0;                      // 视频宽度
    int height = 0;                     // 视频高度
    double frameRate = 0.0;             // 帧率
    std::string codecs;                 // 编解码器信息
    std::string resolution;             // 分辨率字符串 (如 "1920x1080")
    std::string audio;                  // 关联的音频组ID
    std::string video;                  // 关联的视频组ID
    std::string subtitles;              // 关联的字幕组ID
    
    // 解析后的子播放列表 (延迟加载)
    std::shared_ptr<struct HJM3U8Playlist> playlist;
};

/**
 * @brief M3U8 播放列表
 * Builder 模式: 通过解析器逐步构建
 */
struct HJM3U8Playlist {
    using Ptr = std::shared_ptr<HJM3U8Playlist>;
    
    // 基本信息
    int version = 1;                            // EXT-X-VERSION
    int targetDuration = 0;                     // EXT-X-TARGETDURATION (秒)
    int mediaSequence = 0;                      // EXT-X-MEDIA-SEQUENCE
    int discontinuitySequence = 0;              // EXT-X-DISCONTINUITY-SEQUENCE
    
    // 播放列表类型
    bool isMasterPlaylist = false;              // 是否为Master Playlist
    bool isVOD = false;                         // 是否为点播 (有EXT-X-ENDLIST)
    bool isEvent = false;                       // 是否为EVENT类型
    
    // 多分辨率变体 (仅Master Playlist)
    std::vector<HJM3U8Variant::Ptr> variants;
    
    // 分片组 (仅Media Playlist)
    std::vector<HJM3U8SegmentGroup::Ptr> segmentGroups;
    
    // 总时长
    int64_t totalDurationMs = 0;
    
    // 获取所有分片 (扁平化)
    std::vector<HJM3U8Segment> getAllSegments() const {
        std::vector<HJM3U8Segment> result;
        for (const auto& group : segmentGroups) {
            result.insert(result.end(), group->segments.begin(), group->segments.end());
        }
        return result;
    }
    
    // 获取分片总数
    size_t getTotalSegmentCount() const {
        size_t count = 0;
        for (const auto& group : segmentGroups) {
            count += group->segmentCount();
        }
        return count;
    }
    
    // 获取分组数量
    size_t getGroupCount() const {
        return segmentGroups.size();
    }
};

/**
 * @brief M3U8 解析器
 * 
 * Strategy 模式: 根据M3U8类型选择不同的解析策略
 * - Master Playlist: 解析多分辨率变体
 * - Media Playlist: 解析分片并按discontinuity分组
 */
class HJM3U8Parser : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJM3U8Parser>;
    using ParseProgressCallback = std::function<void(int current, int total)>;
    
    HJM3U8Parser() = default;
    virtual ~HJM3U8Parser();
    
    /**
     * @brief 初始化解析器并解析M3U8内容
     * @param mediaUrl 媒体URL
     * @return 成功返回HJ_OK，失败返回错误码
     */
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    
    /**
     * @brief 从字符串内容初始化
     * @param content M3U8文件内容
     * @param baseUrl 基础URL (用于相对路径转换)
     * @return 成功返回HJ_OK，失败返回错误码
     */
    virtual int initFromContent(const std::string& content, const std::string& baseUrl = "");
    
    /**
     * @brief 重置解析器状态
     */
    void reset();
    
    /**
     * @brief 获取解析后的播放列表
     */
    const HJM3U8Playlist::Ptr& getPlaylist() const {
        return m_playlist;
    }
    
    /**
     * @brief 获取所有分片组
     */
    const std::vector<HJM3U8SegmentGroup::Ptr>& getSegmentGroups() const {
        return m_playlist ? m_playlist->segmentGroups : m_emptyGroups;
    }
    
    /**
     * @brief 获取所有变体 (仅Master Playlist有效)
     */
    const std::vector<HJM3U8Variant::Ptr>& getVariants() const {
        return m_playlist ? m_playlist->variants : m_emptyVariants;
    }
    
    /**
     * @brief 是否为Master Playlist
     */
    bool isMasterPlaylist() const {
        return m_playlist && m_playlist->isMasterPlaylist;
    }
    
    /**
     * @brief 获取总时长 (毫秒)
     */
    int64_t getTotalDurationMs() const {
        return m_playlist ? m_playlist->totalDurationMs : 0;
    }
    
    /**
     * @brief 解析所有变体的子播放列表 (仅Master Playlist有效)
     * @param parseAll 是否解析所有变体，false则只解析第一个
     * @return 成功返回HJ_OK，失败返回错误码
     */
    int parseAllVariants(bool parseAll = true);
    
    /**
     * @brief 设置解析进度回调
     */
    void setProgressCallback(ParseProgressCallback callback) {
        m_progressCallback = std::move(callback);
    }
    
    /**
     * @brief 根据时间查找分片
     * @param timeMs 时间 (毫秒)
     * @param outGroupIndex 输出分组索引
     * @param outSegmentIndex 输出分片在组内的索引
     * @return 成功返回true
     */
    bool findSegmentByTime(int64_t timeMs, int& outGroupIndex, int& outSegmentIndex) const;
    
    /**
     * @brief 根据时间获取分片
     * @param timeMs 时间 (毫秒)
     * @return 对应的分片指针，未找到返回nullptr
     */
    const HJM3U8Segment* getSegmentByTime(int64_t timeMs) const;
    
    /**
     * @brief 选择变体 (用于Master Playlist)
     * @param index 变体索引
     * @return 成功返回HJ_OK
     */
    int selectVariant(int index);
    
    /**
     * @brief 根据带宽选择最佳变体
     * @param availableBandwidth 可用带宽 (bps)
     * @return 选中的变体索引，-1表示无变体
     */
    int selectVariantByBandwidth(int availableBandwidth) const;

private:
    // 解析方法
    int parseContent(const std::string& content, const std::string& baseUrl);
    int parseMasterPlaylist(const std::vector<std::string>& lines, const std::string& baseUrl);
    int parseMediaPlaylist(const std::vector<std::string>& lines, const std::string& baseUrl);
    
    // 标签解析辅助方法
    bool parseExtInf(const std::string& line, double& duration, std::string& title);
    bool parseStreamInf(const std::string& line, HJM3U8Variant& variant);
    bool parseKey(const std::string& line, HJM3U8Key& key, const std::string& baseUrl);
    bool parseByteRange(const std::string& line, int64_t& start, int64_t& length);
    
    // 工具方法
    std::string makeAbsoluteUrl(const std::string& baseUrl, const std::string& relativeUrl);
    std::vector<std::string> splitLines(const std::string& content);
    std::string getAttribute(const std::string& line, const std::string& key);
    int parseResolution(const std::string& resolution, int& width, int& height);

private:
    HJMediaUrl::Ptr m_mediaUrl;
    HJXIOContext::Ptr m_xio;
    HJM3U8Playlist::Ptr m_playlist;
    ParseProgressCallback m_progressCallback;
    
    // 空容器用于安全返回引用
    static const std::vector<HJM3U8SegmentGroup::Ptr> m_emptyGroups;
    static const std::vector<HJM3U8Variant::Ptr> m_emptyVariants;
};

NS_HJ_END
