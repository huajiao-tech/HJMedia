//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJSEIWrapper.h"
#include "HJFLog.h"
#include "HJUUIDTools.h"
#include <algorithm>
#include <iterator>
#include <cctype>

NS_HJ_BEGIN
//***********************************************************************************//
namespace {
// H.264 NAL unit type for SEI
constexpr uint8_t H264_NAL_SEI = 6;

// H.265 NAL unit types for SEI
constexpr uint8_t H265_NAL_PREFIX_SEI = 39;
constexpr uint8_t H265_NAL_SUFFIX_SEI = 40;

// user_data_unregistered payload type
constexpr uint8_t SEI_TYPE_USER_UNREG = 5;

// Annex B start code (4-byte)
constexpr uint8_t START_CODE4[4] = {0x00, 0x00, 0x00, 0x01};

inline void writeSeiCode(HJRawBuffer& out, size_t value)
{
    while (value >= 255) { out.push_back(0xFF); value -= 255; }
    out.push_back(static_cast<uint8_t>(value));
}
}

//------------------------------------------------------------------------------------//
HJRawBuffer HJSEIWrapper::makeSEINal(const std::vector<HJSEIData>& datas, bool isH265)
{
    HJRawBuffer out;
    if (datas.empty()) {
        HJFLogw("No SEI data to add.");
        return out;
    }

    // 逐条封装为独立SEI NAL, 并顺次拼接
    for (const auto& it : datas) {
        if (!packOneSEI(it, isH265, out, HJNALFormat::AnnexB)) {
            // 任意一条失败则跳过该条, 继续后续
            continue;
        }
    }
    return out;
}

HJRawBuffer HJSEIWrapper::makeSEINal(const std::vector<HJSEIData>& datas, bool isH265, HJNALFormat fmt)
{
    HJRawBuffer out;
    if (datas.empty()) return out;
    for (const auto& it : datas) {
        if (!packOneSEI(it, isH265, out, fmt)) continue;
    }
    return out;
}

HJRawBuffer HJSEIWrapper::makeSEINalMerged(const std::vector<HJSEIData>& datas, bool isH265, HJNALFormat fmt)
{
    HJRawBuffer out;
    if (datas.empty()) return out;
    packMergedSEI(datas, isH265, out, fmt);
    return out;
}

//------------------------------------------------------------------------------------//
std::vector<HJSEIData> HJSEIWrapper::parseSEINals(const HJRawBuffer& nalunits)
{
    std::vector<HJSEIData> result;
    return parseSEINals(nalunits, [&](const HJSEIData& item){ result.push_back(item); }), result;
}

//------------------------------------------------------------------------------------//
std::vector<HJSEIData> HJSEIWrapper::parseSEINals(const HJRawBuffer& nalunits,
                                           const std::function<void(const HJSEIData&)> onItem)
{
    std::vector<HJSEIData> collected;
    if (nalunits.empty()) return collected;

    const uint8_t* p = nalunits.data();
    const uint8_t* end = p + nalunits.size();
    const size_t nalsSize = nalunits.size();

    //auto isAnnexB = [&](const uint8_t* data, size_t size) {
    //    if (size < 3) return false;
    //    // if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) return true;
    //    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) return true;
    //    return false;
    //};
    bool annexB = isAnnexB(p, nalsSize);

    if (annexB) {
        // Annex B 格式处理 (起始码格式)
        while (p < end) {
            const uint8_t* sc = findStartCode(p, end);
            if (sc == end) break; // no more

            // 跳过起始码 (3或4字节)
            const uint8_t* nalStart = sc + 3;
            if (nalStart < end && *nalStart == 0x01) ++nalStart; // 4字节
            if (nalStart >= end) break;

            // 找下一个起始码以确定本NAL结束
            const uint8_t* nextSC = findStartCode(nalStart, end);
            const uint8_t* nalEnd = nextSC;
            if (nalStart >= nalEnd) { p = nalEnd; continue; }

            // 自动识别H.264/H.265 SEI
            bool isH265 = false;
            uint8_t nalType = 0;
            const uint8_t* payload = nullptr;

            // 尝试按H.265解析头(至少2字节)
            if (nalEnd - nalStart >= 2) {
                uint8_t t = (nalStart[0] >> 1) & 0x3F;
                if (t == H265_NAL_PREFIX_SEI || t == H265_NAL_SUFFIX_SEI) {
                    isH265 = true; nalType = t; payload = nalStart + 2;
                }
            }
            // 否则尝试H.264
            if (!isH265) {
                nalType = nalStart[0] & 0x1F;
                if (nalType == H264_NAL_SEI) payload = nalStart + 1; else payload = nullptr;
            }

            if (payload && payload < nalEnd) {
                HJRawBuffer rbsp;
                ebspToRbsp(payload, static_cast<size_t>(nalEnd - payload), rbsp);

                // 去rbsp_trailing_bits: 去末尾0后, 如最后一字节为0x80则去掉
                size_t sz = rbsp.size();
                while (sz > 0 && rbsp[sz - 1] == 0x00) --sz;
                if (sz > 0 && rbsp[sz - 1] == 0x80) --sz;

                if (sz > 0) {
                    parseSEIFromRbsp(rbsp.data(), rbsp.data() + sz, collected, onItem);
                }
            }
            p = nalEnd;
        }
    } else {
        // AVCC 格式处理 (长度前缀格式)
        while (p + 4 <= end) {
            // 读取4字节大端序长度前缀
            uint32_t nalLength = (static_cast<uint32_t>(p[0]) << 24) |
                                 (static_cast<uint32_t>(p[1]) << 16) |
                                 (static_cast<uint32_t>(p[2]) << 8) |
                                 static_cast<uint32_t>(p[3]);
            
            p += 4; // 跳过长度字段
            
            // 检查数据完整性
            if (p + nalLength > end) {
                HJFLogw("AVCC NAL length exceeds buffer size, truncated data.");
                break;
            }

            const uint8_t* nalStart = p;
            const uint8_t* nalEnd = p + nalLength;

            if (nalLength == 0) {
                continue; // 跳过空 NAL
            }

            // 自动识别H.264/H.265 SEI
            bool isH265 = false;
            uint8_t nalType = 0;
            const uint8_t* payload = nullptr;

            // 尝试按H.265解析头(至少2字节)
            if (nalLength >= 2) {
                uint8_t t = (nalStart[0] >> 1) & 0x3F;
                if (t == H265_NAL_PREFIX_SEI || t == H265_NAL_SUFFIX_SEI) {
                    isH265 = true; nalType = t; payload = nalStart + 2;
                }
            }
            // 否则尝试H.264
            if (!isH265 && nalLength >= 1) {
                nalType = nalStart[0] & 0x1F;
                if (nalType == H264_NAL_SEI) payload = nalStart + 1; else payload = nullptr;
            }

            if (payload && payload < nalEnd) {
                HJRawBuffer rbsp;
                ebspToRbsp(payload, static_cast<size_t>(nalEnd - payload), rbsp);

                // 去rbsp_trailing_bits: 去末尾0后, 如最后一字节为0x80则去掉
                size_t sz = rbsp.size();
                while (sz > 0 && rbsp[sz - 1] == 0x00) --sz;
                if (sz > 0 && rbsp[sz - 1] == 0x80) --sz;

                if (sz > 0) {
                    parseSEIFromRbsp(rbsp.data(), rbsp.data() + sz, collected, onItem);
                }
            }

            p = nalEnd; // 移动到下一个 NAL 单元
        }
    }
    return collected;
}

const bool HJSEIWrapper::isAnnexB(const uint8_t* data, size_t size)
{
    if (size < 4) return false;
    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) 
    {
        uint32_t nalLength = (static_cast<uint32_t>(data[0]) << 24) |
            (static_cast<uint32_t>(data[1]) << 16) |
            (static_cast<uint32_t>(data[2]) << 8) |
            static_cast<uint32_t>(data[3]);
        if (nalLength + 4 == size) {
            return false;
        }
        return true;
    }
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) return true;
    return false;
}

//------------------------------------------------------------------------------------//
void HJSEIWrapper::ebspToRbsp(const uint8_t* payload, size_t size, HJRawBuffer& rbsp)
{
    rbsp.clear(); rbsp.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        if (i + 2 < size && payload[i] == 0x00 && payload[i + 1] == 0x00 && payload[i + 2] == 0x03) {
            rbsp.push_back(0x00);
            rbsp.push_back(0x00);
            i += 2; // skip 0x03
        } else {
            rbsp.push_back(payload[i]);
        }
    }
}

//------------------------------------------------------------------------------------//
HJRawBuffer HJSEIWrapper::rbspToEbsp(const HJRawBuffer& rbsp)
{
    HJRawBuffer ebsp; ebsp.reserve(rbsp.size() * 3 / 2);
    int zeroCount = 0;
    for (uint8_t b : rbsp) {
        if (zeroCount == 2 && (b <= 0x03)) {
            ebsp.push_back(0x03); // 插入防竞争字节
            zeroCount = 0;
        }
        ebsp.push_back(b);
        zeroCount = (b == 0x00) ? (zeroCount + 1) : 0;
    }
    return ebsp;
}

//------------------------------------------------------------------------------------//
const uint8_t* HJSEIWrapper::findStartCode(const uint8_t* p, const uint8_t* end)
{
    // 搜索 00 00 01 或 00 00 00 01
    while (p + 3 <= end) {
        if (p[0] == 0x00 && p[1] == 0x00) {
            //if (p + 3 <= end && p[2] == 0x01) return p; // 3字节
            if (p + 4 <= end && p[2] == 0x00 && p[3] == 0x01) return p; // 4字节
        }
        ++p;
    }
    return end;
}

//------------------------------------------------------------------------------------//
bool HJSEIWrapper::packOneSEI(const HJSEIData& item, bool isH265, HJRawBuffer& out, HJNALFormat fmt)
{
    // 构造RBSP: [payloadType][payloadSize][payload] ... + rbsp_trailing_bits
    auto uuid16 = HJUtilitys::parseUuidTo16Bytes(item.uuid);
    if (uuid16.size() != 16) return false;

    HJRawBuffer rbsp;
    rbsp.reserve(32 + item.data.size() + 8);

    // 仅一条 user_data_unregistered 消息
    writeSeiCode(rbsp, SEI_TYPE_USER_UNREG);                       // payloadType
    writeSeiCode(rbsp, 16 + item.data.size());                     // payloadSize
    rbsp.insert(rbsp.end(), uuid16.begin(), uuid16.end());         // uuid
    rbsp.insert(rbsp.end(), item.data.begin(), item.data.end());   // data

    // rbsp_trailing_bits: 1后若干0 => 字节对齐直接 0x80
    rbsp.push_back(0x80);

    // RBSP -> EBSP
    auto ebsp = rbspToEbsp(rbsp);

    if (fmt == HJNALFormat::AnnexB) {
        // Annex B: start code + header + payload
        out.insert(out.end(), std::begin(START_CODE4), std::end(START_CODE4));
        if (isH265) {
            uint8_t b0 = static_cast<uint8_t>((H265_NAL_PREFIX_SEI & 0x3F) << 1);
            uint8_t b1 = 0x01;
            out.push_back(b0);
            out.push_back(b1);
        } else {
            out.push_back(H264_NAL_SEI);
        }
        out.insert(out.end(), ebsp.begin(), ebsp.end());
    } else { // AVCC length-prefixed
        size_t nal_size = ebsp.size() + (isH265 ? 2 : 1);
        uint8_t len_be[4] = {
            static_cast<uint8_t>((nal_size >> 24) & 0xFF),
            static_cast<uint8_t>((nal_size >> 16) & 0xFF),
            static_cast<uint8_t>((nal_size >> 8) & 0xFF),
            static_cast<uint8_t>((nal_size) & 0xFF)
        };
        out.insert(out.end(), std::begin(len_be), std::end(len_be));
        if (isH265) {
            uint8_t b0 = static_cast<uint8_t>((H265_NAL_PREFIX_SEI & 0x3F) << 1);
            uint8_t b1 = 0x01;
            out.push_back(b0);
            out.push_back(b1);
        } else {
            out.push_back(H264_NAL_SEI);
        }
        out.insert(out.end(), ebsp.begin(), ebsp.end());
    }
    return true;
}
bool HJSEIWrapper::packMergedSEI(const std::vector<HJSEIData>& datas, bool isH265, HJRawBuffer& out, HJNALFormat fmt)
{
    // Build one RBSP containing multiple messages
    HJRawBuffer rbsp;
    for (const auto& d : datas) {
        auto uuid16 = HJUtilitys::parseUuidTo16Bytes(d.uuid);
        if (uuid16.size() != 16) continue;
        writeSeiCode(rbsp, SEI_TYPE_USER_UNREG);
        writeSeiCode(rbsp, 16 + d.data.size());
        rbsp.insert(rbsp.end(), uuid16.begin(), uuid16.end());
        rbsp.insert(rbsp.end(), d.data.begin(), d.data.end());
    }
    if (rbsp.empty()) return false;
    rbsp.push_back(0x80);
    auto ebsp = rbspToEbsp(rbsp);

    if (fmt == HJNALFormat::AnnexB) {
        out.insert(out.end(), std::begin(START_CODE4), std::end(START_CODE4));
        if (isH265) {
            uint8_t b0 = static_cast<uint8_t>((H265_NAL_PREFIX_SEI & 0x3F) << 1);
            uint8_t b1 = 0x01;
            out.push_back(b0);
            out.push_back(b1);
        } else {
            out.push_back(H264_NAL_SEI);
        }
        out.insert(out.end(), ebsp.begin(), ebsp.end());
    } else {
        size_t nal_size = ebsp.size() + (isH265 ? 2 : 1);
        uint8_t len_be[4] = {
            static_cast<uint8_t>((nal_size >> 24) & 0xFF),
            static_cast<uint8_t>((nal_size >> 16) & 0xFF),
            static_cast<uint8_t>((nal_size >> 8) & 0xFF),
            static_cast<uint8_t>((nal_size) & 0xFF)
        };
        out.insert(out.end(), std::begin(len_be), std::end(len_be));
        if (isH265) {
            uint8_t b0 = static_cast<uint8_t>((H265_NAL_PREFIX_SEI & 0x3F) << 1);
            uint8_t b1 = 0x01;
            out.push_back(b0);
            out.push_back(b1);
        } else {
            out.push_back(H264_NAL_SEI);
        }
        out.insert(out.end(), ebsp.begin(), ebsp.end());
    }
    return true;
}

//------------------------------------------------------------------------------------//
void HJSEIWrapper::parseSEIFromRbsp(const uint8_t* rbsp, const uint8_t* rbspEnd,
                             std::vector<HJSEIData>& out,
                             const std::function<void(const HJSEIData&)>& onItem)
{
    const uint8_t* p = rbsp;
    while (p < rbspEnd) {
        // payloadType
        size_t payloadType = 0;
        while (p < rbspEnd && *p == 0xFF) { payloadType += 255; ++p; }
        if (p >= rbspEnd) break;
        payloadType += *p++;

        // payloadSize
        size_t payloadSize = 0;
        while (p < rbspEnd && *p == 0xFF) { payloadSize += 255; ++p; }
        if (p >= rbspEnd) break;
        payloadSize += *p++;

        if (p + payloadSize > rbspEnd) break; // 不完整

        if (payloadType == SEI_TYPE_USER_UNREG && payloadSize >= 16) {
            HJSEIData item;
            // 以hex字符串输出UUID, 避免返回不可见字符
            item.uuid = HJUtilitys::uuidBytesToHex(p);
            item.data.assign(p + 16, p + payloadSize);
            if (onItem) onItem(item);
            out.push_back(std::move(item));
        }
        p += payloadSize;
    }
}

//------------------------------------------------------------------------------------//
std::vector<uint8_t> HJSEIWrapper::insertSEIToFrame(const uint8_t* nals, size_t nalsSize, const std::vector<HJRawBuffer>& seiNals, bool isH265)
{
    if (!nals || nalsSize == 0) return {};
    if (seiNals.empty()) return std::vector<uint8_t>(nals, nals + nalsSize);

    bool insertHeader = false;
    // 简单的格式检测 lambda
    //auto isAnnexB = [&](const uint8_t* data, size_t size) {
    //    if (size < 3) return false;
    //    //if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) return true;
    //    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) return true;
    //    return false;
    //};

    // NAL类型获取 lambda
    auto getNalType = [&](const uint8_t* nalStart) {
        if (isH265) return static_cast<uint8_t>((nalStart[0] >> 1) & 0x3F);
        return static_cast<uint8_t>(nalStart[0] & 0x1F);
    };

    // 检查是否为 EOS/EOSeq lambda
    auto isEndNal = [&](uint8_t type) {
        if (isH265) return type == 36 || type == 37; // EOS or EOSeq
        return type == 10 || type == 11; // EOS or EOSeq
    };

    bool annexB = isAnnexB(nals, nalsSize);
    size_t insertPos = nalsSize; // 默认追加到末尾

    if (annexB) {
        const uint8_t* p = nals;
        const uint8_t* end = nals + nalsSize;
        while (p < end) {
            const uint8_t* sc = findStartCode(p, end);
            if (sc == end) break;

            const uint8_t* nalStart = sc + 3;
            if (nalStart < end && *nalStart == 0x01) nalStart++; // 4字节SC

            if (nalStart >= end) break;

            uint8_t type = getNalType(nalStart);
            if (isEndNal(type)) {
                insertPos = sc - nals;
                break;
            }
            // 移动到下一个可能的位置，避免死循环，至少前进1字节
            p = nalStart;
        }
    } else {
        // Length Prefix
        size_t offset = 0;
        while (offset + 4 <= nalsSize) {
            uint32_t len = (static_cast<uint32_t>(nals[offset]) << 24) |
                           (static_cast<uint32_t>(nals[offset+1]) << 16) |
                           (static_cast<uint32_t>(nals[offset+2]) << 8) |
                           static_cast<uint32_t>(nals[offset+3]);
            
            if (offset + 4 + len > nalsSize) break; // 数据不完整

            const uint8_t* nalStart = nals + offset + 4;
            uint8_t type = getNalType(nalStart);
            if (isEndNal(type)) {
                insertPos = offset;
                break;
            }
            offset += 4 + len;
        }
    }

    // 构建结果
    std::vector<uint8_t> result;
    result.reserve(nalsSize + seiNals.size() * 128);

    // 1. 插入点之前的数据
    result.insert(result.end(), nals, nals + insertPos);

    // 2. 插入SEI
    for (const auto& sei : seiNals) 
    {
        if (annexB) {
            if (insertHeader) {
                uint8_t sc[4] = { 0x00, 0x00, 0x00, 0x01 };
                result.insert(result.end(), std::begin(sc), std::end(sc));
            }
            result.insert(result.end(), sei.begin(), sei.end());
        } else 
        {
            if (insertHeader) {
                uint32_t len = static_cast<uint32_t>(sei.size());
                uint8_t lenBytes[4] = {
                    static_cast<uint8_t>((len >> 24) & 0xFF),
                    static_cast<uint8_t>((len >> 16) & 0xFF),
                    static_cast<uint8_t>((len >> 8) & 0xFF),
                    static_cast<uint8_t>(len & 0xFF)
                };
                result.insert(result.end(), std::begin(lenBytes), std::end(lenBytes));
            }
            result.insert(result.end(), sei.begin(), sei.end());
        }
    }

    // 3. 插入点之后的数据
    result.insert(result.end(), nals + insertPos, nals + nalsSize);

    return result;
}

std::vector<HJRawBuffer> HJSEIWrapper::extractSEIFromFrame(const uint8_t* nals, size_t nalsSize, bool isH265)
{
    std::vector<HJRawBuffer> result;
    if (!nals || nalsSize == 0) return result;

    //auto isAnnexB = [&](const uint8_t* data, size_t size) {
    //    if (size < 3) return false;
    //    //if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) return true;
    //    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) return true;
    //    return false;
    //};

    auto getNalType = [&](const uint8_t* nalStart) {
        if (isH265) return static_cast<uint8_t>((nalStart[0] >> 1) & 0x3F);
        return static_cast<uint8_t>(nalStart[0] & 0x1F);
    };

    auto isSeiNal = [&](uint8_t type) {
        if (isH265) return type == 39 || type == 40; // Prefix or Suffix SEI
        return type == 6; // SEI
    };

    bool annexB = isAnnexB(nals, nalsSize);

    if (annexB) {
        const uint8_t* p = nals;
        const uint8_t* end = nals + nalsSize;
        while (p < end) {
            const uint8_t* sc = findStartCode(p, end);
            if (sc == end) break;

            const uint8_t* nalStart = sc + 3;
            if (nalStart < end && *nalStart == 0x01) nalStart++;

            if (nalStart >= end) break;

            const uint8_t* nextSC = findStartCode(nalStart, end);
            
            uint8_t type = getNalType(nalStart);
            if (isSeiNal(type)) {
                result.emplace_back(sc, nextSC);
            }
            p = nextSC;
        }
    } else {
        size_t offset = 0;
        while (offset + 4 <= nalsSize) {
            uint32_t len = (static_cast<uint32_t>(nals[offset]) << 24) |
                           (static_cast<uint32_t>(nals[offset+1]) << 16) |
                           (static_cast<uint32_t>(nals[offset+2]) << 8) |
                           static_cast<uint32_t>(nals[offset+3]);
            
            if (offset + 4 + len > nalsSize) break;

            const uint8_t* nalStart = nals + offset + 4;
            uint8_t type = getNalType(nalStart);
            if (isSeiNal(type)) {
                result.emplace_back(nals + offset, nals + offset + 4 + len);
            }
            offset += 4 + len;
        }
    }
    return result;
}

NS_HJ_END
