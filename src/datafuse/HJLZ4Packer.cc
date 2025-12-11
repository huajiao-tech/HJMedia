//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJLZ4Packer.h"
#include "HJFLog.h"
#include <algorithm>
#include <limits>
#include "lz4.h"
NS_HJ_BEGIN
//***********************************************************************************//
bool HJLZ4Packer::compressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst)
{
    dst.clear();
    if (src.empty()) return true;
    int srcSize = static_cast<int>(src.size());
    int maxDst = LZ4_compressBound(srcSize);
    if (maxDst <= 0) {
        HJFLoge("HJLZ4Codec::compressOne: LZ4_compressBound failed.");
        return false;
    }
    dst.resize(static_cast<size_t>(maxDst));
    int written = LZ4_compress_default(reinterpret_cast<const char*>(src.data()),
                                       reinterpret_cast<char*>(dst.data()),
                                       srcSize, maxDst);
    if (written <= 0) { 
        dst.clear();
        HJFLogw("HJLZ4Codec::compressOne: LZ4_compress_default failed.");
        return false; 
    }
    dst.resize(static_cast<size_t>(written));
    return true;
}

bool HJLZ4Packer::decompressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, int expected_size)
{
    dst.clear();
    if (expected_size < 0) return false;
    if (expected_size == 0) { dst.clear(); return true; }
    dst.resize(static_cast<size_t>(expected_size));
    int dec = LZ4_decompress_safe(reinterpret_cast<const char*>(src.data()),
                                  reinterpret_cast<char*>(dst.data()),
                                  static_cast<int>(src.size()), expected_size);
    if (dec < 0 || dec != expected_size) { 
        dst.clear(); 
        HJFLogw("HJLZ4Codec::decompressOne: LZ4_decompress_safe failed. dec:{}, expected_size:{}", dec, expected_size);
        return false; 
    }
    return true;
}

HJVibeData::Ptr HJLZ4Packer::pack(const HJVibeData::Ptr& data)
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
            HJFLoge("HJLZ4Packer::pack compressOne failed at index: {}", i);
            return nullptr;
        }
        origin_size += src->getData().size();

        auto userData = HJUserData::make(dst);
        userData->setOriginSize(origin_size/*src->getOriginSize()*/);
        vibeData->append(userData);   
    }
    (*vibeData)["compress"] = (int)HJCompressType::COMPRESS_LZ4; //std::string("lz4");
    (*vibeData)["origin_size"] = origin_size;

    return vibeData;
}

HJVibeData::Ptr HJLZ4Packer::unpack(const HJVibeData::Ptr& data)
{
    if(!data) {
        return nullptr;
    }
    HJVibeData::Ptr vibeData = std::make_shared<HJVibeData>();
    for(size_t i = 0; i < data->size(); i++) {
        const auto& src = data->get(i);
        std::vector<uint8_t> dst;
        if(!decompressOne(src->getData(), dst, src->getOriginSize())) {
            HJFLoge("HJLZ4Packer::unpack decompressOne failed at index: {}", i);
            return nullptr;
        }
        auto userData = HJUserData::make(dst);
        userData->setOriginSize(dst.size());
        vibeData->append(userData);
    }   
    return vibeData;
}


NS_HJ_END