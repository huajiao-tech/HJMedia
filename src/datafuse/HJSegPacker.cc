//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJSegPacker.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJVibeData::Ptr HJSegPacker::pack(const HJVibeData::Ptr& data)
{
    if(!data) {
        return nullptr;
    }
    HJVibeData::Ptr outVibeData = std::make_shared<HJVibeData>();
    for(size_t i = 0; i < data->size(); ++i) {
        auto segs = split(data->get(i));
        for(auto& seg : segs) {
            outVibeData->append(seg);
        }
    }
    outVibeData->clone(data);

    return outVibeData;
}

HJVibeData::Ptr HJSegPacker::unpack(const HJVibeData::Ptr& data)
{
    if(!data) {
        return nullptr;
    }
    HJVibeData::Ptr outVibeData = std::make_shared<HJVibeData>();
    std::vector<HJUserData::Ptr> currentSegs;
    currentSegs.reserve(data->size());
    auto flushCurrent = [&]() {
        if (currentSegs.empty()) {
            return;
        }
        auto out = combine(currentSegs);
        if (out) {
            out->setOriginSize(data->getSizet("origin_size"));
            outVibeData->append(out);
        }
        currentSegs.clear();
    };
    size_t lastSegId = 0;
    bool hasSegId = false;
    for (const auto& seg : data->getData()) {
        if (!seg) {
            HJFLogw("HJSegPacker::unpack encountered null segment");
            continue;
        }
        if (!currentSegs.empty() && hasSegId && seg->getID() <= lastSegId) {
            flushCurrent();
            hasSegId = false;
        }
        currentSegs.push_back(seg);
        lastSegId = seg->getID();
        hasSegId = true;
    }
    flushCurrent();

    outVibeData->clone(data);
    return outVibeData;
}

std::vector<HJUserData::Ptr> HJSegPacker::split(const HJUserData::Ptr& data)
{
    if (!data) {
        return {};
    }
    if(data->size() <= m_kMaxSegSize) {
        return {data};
    }
    size_t total = (data->size() + m_kMaxSegSize - 1) / m_kMaxSegSize;
    if (total > 255) {
        HJFLoge("Segment count limit exceeded: {}", total);
        return {};
    }
    std::vector<HJUserData::Ptr> out;
    out.reserve(total);
    for (size_t i = 0; i < total; ++i) {
        size_t off = i * m_kMaxSegSize;
        size_t len = std::min(m_kMaxSegSize, data->size() - off);
        std::vector<uint8_t> setData(data->getData().begin() + off, data->getData().begin() + off + len);
        auto seg = HJUserData::make(setData);
        seg->setID(i);
        out.emplace_back(seg);
    }
    return out;
}

HJUserData::Ptr HJSegPacker::combine(const std::vector<HJUserData::Ptr>& segs)
{
    if (segs.empty()) return nullptr;
    std::vector<std::pair<size_t, HJUserData::Ptr>> tmpSegs;
    tmpSegs.reserve(segs.size());
    for (const auto& s : segs) {
        if (s->getID() >= segs.size()) {
            HJFLoge("Invalid segment id: {}", s->getID());
            return {};
        }
        tmpSegs.emplace_back(s->getID(), s);
    }
    std::sort(tmpSegs.begin(), tmpSegs.end(), [](const std::pair<size_t, HJUserData::Ptr>& a, const std::pair<size_t, HJUserData::Ptr>& b){ return a.first < b.first; });
    
    for (size_t i = 0; i < tmpSegs.size(); ++i) {
        if (tmpSegs[i].first != i) {
            HJFLoge("Invalid segment id: {}", tmpSegs[i].first);
            return {};
        }
    }

    size_t totalSize = 0;
    for (const auto& s : tmpSegs) totalSize += s.second->size();
    std::vector<uint8_t> outData;
    outData.reserve(totalSize);
    for (const auto& s : tmpSegs) {
        outData.insert(outData.end(), s.second->getData().begin(), s.second->getData().end());
    }

    return HJUserData::make(outData);
}

NS_HJ_END
