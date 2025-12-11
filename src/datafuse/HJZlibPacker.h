//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <cstdint>
#include "HJUtilitys.h"
#include "HJBasePacker.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJZlibPacker : public HJBasePacker {
public:
    HJ_DECLARE_PUWTR(HJZlibPacker);

    HJZlibPacker(const HJParams::Ptr& params = nullptr) 
        : HJBasePacker(params) {
        setName("HJZlibPacker");
    }

    virtual HJVibeData::Ptr pack(const HJVibeData::Ptr& data) override;
    virtual HJVibeData::Ptr unpack(const HJVibeData::Ptr& data) override;
private:
    static bool compressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst);
    static bool decompressOne(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, int expected_size);
};

NS_HJ_END
