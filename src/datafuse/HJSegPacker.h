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
class HJSegPacker : public HJBasePacker {
public:
    HJ_DECLARE_PUWTR(HJSegPacker);
    HJSegPacker(const HJParams::Ptr& params = nullptr, size_t maxSegSize = HJ_SEI_SEG_MAX_SIZE) 
        : HJBasePacker(params)
        , m_kMaxSegSize(maxSegSize) 
    {
        setName("HJSegPacker");
    }

    virtual HJVibeData::Ptr pack(const HJVibeData::Ptr& data) override;
    virtual HJVibeData::Ptr unpack(const HJVibeData::Ptr& data) override;
private:
    std::vector<HJUserData::Ptr> split(const HJUserData::Ptr& data);
    HJUserData::Ptr combine(const std::vector<HJUserData::Ptr>& segs);
private:
    size_t m_kMaxSegSize = HJ_SEI_SEG_MAX_SIZE;
};

NS_HJ_END