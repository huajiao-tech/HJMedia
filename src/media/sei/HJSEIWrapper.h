//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include "HJUtilitys.h"

NS_HJ_BEGIN
// SEI pack/unpack helper for H.264/H.265 (user_data_unregistered)
//***********************************************************************************//
// 用户数据结构体: uuid + 载荷数据
typedef struct HJSEIData {
    std::string uuid;           // 16字节原始UUID或32/36位hex字符串
    HJRawBuffer data;            // 具体数据
} HJSEIData;

class HJSEINals : public HJObject {
public:
    HJ_DECLARE_PUWTR(HJSEINals);

    HJSEINals() = default;

    void addData(const HJRawBuffer& data) {
        m_datas.emplace_back(data);
    }
    void addData(const std::string& data) {
        addData(HJRawBuffer(data.begin(), data.end()));
    }
    void addData(const uint8_t* data, size_t size) {
        addData(HJRawBuffer(data, data + size));
    }

    const std::vector<HJRawBuffer>& getDatas() const { return m_datas; }

    bool empty() const { return m_datas.empty(); }
    size_t size() const { return m_datas.size(); }
public:
    std::vector<HJRawBuffer> m_datas;
};

//***********************************************************************************//
// SEI封装/解析类: 仅处理 user_data_unregistered (payloadType=5)
class HJSEIWrapper final {
public:
    HJ_DECLARE_PUWTR(HJSEIWrapper);
    // 输出封装格式
    enum class HJNALFormat { AnnexB, AVCC };

    // 封装: 输入多个用户数据, 输出按Annex B拼接的SEI NAL字节流
    // - 每个HJSEIData独立封装为一个SEI NAL (便于流式发送)
    // - 返回值为多个NAL串联后的字节流(每个NAL前含0x00000001起始码)
    // - isH265=true 生成H.265前缀SEI(nal_unit_type=39); 否则为H.264(nal_unit_type=6)
    static HJRawBuffer makeSEINal(const std::vector<HJSEIData>& datas, bool isH265 = true);
    // 指定输出格式（AnnexB或AVCC），逐条封装为独立SEI NAL
    static HJRawBuffer makeSEINal(const std::vector<HJSEIData>& datas, bool isH265, HJNALFormat fmt);

    // 合并多消息到一个NAL（提高打包效率），可选AnnexB或AVCC输出
    static HJRawBuffer makeSEINalMerged(const std::vector<HJSEIData>& datas,
                                                 bool isH265 = true,
                                                 HJNALFormat fmt = HJNALFormat::AnnexB);

    // 解析: 输入若干NAL串联的字节流, 提取所有user_data_unregistered
    // - 自动按Annex B起始码分段; 自动识别H.264(6)/H.265(39/40) SEI
    // - 返回解析出的所有HJSEIData
    static std::vector<HJSEIData> parseSEINals(const HJRawBuffer& nalunits);

    // 可选: 解析时回调每条SEI (不改变返回值的版本, 方便流式消费)
    static std::vector<HJSEIData> parseSEINals(const HJRawBuffer& nalunits,
                                               const std::function<void(const HJSEIData&)> onItem);

    static std::vector<uint8_t> insertSEIToFrame(const uint8_t* nals, size_t nalsSize, const std::vector<HJRawBuffer>& seiNals, bool isH265 = true);
    static std::vector<HJRawBuffer> extractSEIFromFrame(const uint8_t* nals, size_t nalsSize, bool isH265 = true);
private:
    static const bool isAnnexB(const uint8_t* data, size_t size);
    // EBSP<->RBSP转换
    static void ebspToRbsp(const uint8_t* payload, size_t size, HJRawBuffer& rbsp);
    static HJRawBuffer rbspToEbsp(const HJRawBuffer& rbsp);

    // 起始码扫描
    static const uint8_t* findStartCode(const uint8_t* p, const uint8_t* end);

    // 单条SEI打包为一帧NAL(含起始码+头), 追加到out
    static bool packOneSEI(const HJSEIData& item, bool isH265, HJRawBuffer& out, HJNALFormat fmt);
    static bool packMergedSEI(const std::vector<HJSEIData>& datas, bool isH265, HJRawBuffer& out, HJNALFormat fmt);

    // 解析一个SEI NAL (输入已确定是SEI; 传入payload起始与结束指针)
    static void parseSEIFromRbsp(const uint8_t* rbsp, const uint8_t* rbspEnd,
                                 std::vector<HJSEIData>& out,
                                 const std::function<void(const HJSEIData&)>& onItem);
};

NS_HJ_END

