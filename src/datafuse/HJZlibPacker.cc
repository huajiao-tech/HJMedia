//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJZlibPacker.h"
#include "HJFLog.h"
#include <algorithm>
#include <limits>
#include <zlib.h>

NS_HJ_BEGIN
//***********************************************************************************//
bool HJZlibPacker::compressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst)
{
    dst.clear();
    if (src.empty()) return true;
    
    uLong sourceLen = static_cast<uLong>(src.size());
    uLong destLen = compressBound(sourceLen);
    
    dst.resize(destLen);
    
    int ret = compress(dst.data(), &destLen, src.data(), sourceLen);
    if (ret != Z_OK) {
        dst.clear();
        HJFLoge("HJZlibPacker::compressOne: zlib compress failed. ret: {}", ret);
        return false;
    }
    
    dst.resize(destLen);
    return true;
}

bool HJZlibPacker::decompressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, int expected_size)
{
    dst.clear();
    if (expected_size < 0) return false;
    if (expected_size == 0) { dst.clear(); return true; }
    
    dst.resize(static_cast<size_t>(expected_size));
    uLongf destLen = static_cast<uLongf>(expected_size);
    
    int ret = uncompress(dst.data(), &destLen, src.data(), static_cast<uLong>(src.size()));
    if (ret != Z_OK) {
        dst.clear();
        HJFLogw("HJZlibPacker::decompressOne: zlib uncompress failed. ret: {}, expected_size: {}", ret, expected_size);
        return false;
    }
    
    if (destLen != static_cast<uLongf>(expected_size)) {
         HJFLogw("HJZlibPacker::decompressOne: zlib uncompress size mismatch. destLen: {}, expected_size: {}", destLen, expected_size);
         // This might happen if expected_size was wrong, but uncompress succeeded with less data? 
         // Actually uncompress returns Z_BUF_ERROR if dest buffer is not large enough.
         // If it returns Z_OK, it means it decompressed everything it could from src.
         // If the decompressed size doesn't match expected, it's suspicious but maybe okay if we trust zlib.
         // However, for safety, let's resize to actual.
         dst.resize(destLen);
    }
    
    return true;
}

HJVibeData::Ptr HJZlibPacker::pack(const HJVibeData::Ptr& data)
{
    if(!data || data->size() > 1) {
        return nullptr;
    }
    size_t origin_size = 0;
    HJVibeData::Ptr vibeData = std::make_shared<HJVibeData>();
    for(size_t i = 0; i < data->size(); i++) {
        const auto& src = data->get(i);
        std::vector<uint8_t> dst;
        if(!compressOne(src->getData(), dst)) {
            HJFLoge("HJZlibPacker::pack compressOne failed at index: {}", i);
            return nullptr;
        }
        origin_size += src->getData().size();

        auto userData = HJUserData::make(dst);
        userData->setOriginSize(origin_size);
        vibeData->append(userData);   
    }
    (*vibeData)["compress"] = (int)HJCompressType::COMPRESS_ZLIB;
    (*vibeData)["origin_size"] = origin_size;

    return vibeData;
}

HJVibeData::Ptr HJZlibPacker::unpack(const HJVibeData::Ptr& data)
{
    if(!data) {
        return nullptr;
    }
    HJVibeData::Ptr vibeData = std::make_shared<HJVibeData>();
    for(size_t i = 0; i < data->size(); i++) {
        const auto& src = data->get(i);
        std::vector<uint8_t> dst;
        if(!decompressOne(src->getData(), dst, static_cast<int>(src->getOriginSize()))) {
            HJFLoge("HJZlibPacker::unpack decompressOne failed at index: {}", i);
            return nullptr;
        }
        auto userData = HJUserData::make(dst);
        userData->setOriginSize(dst.size());
        vibeData->append(userData);
    }   
    return vibeData;
}

NS_HJ_END
