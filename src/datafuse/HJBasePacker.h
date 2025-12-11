//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <cstdint>
#include "HJUtilitys.h"
#include "HJXVibeTypes.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJBasePacker : public HJObject {
public:
    HJ_DECLARE_PUWTR(HJBasePacker);
    HJBasePacker(const HJParams::Ptr& params = nullptr) : m_params(params) {}
    virtual ~HJBasePacker() {}
    
    HJBasePacker(const HJBasePacker&) = delete;
    HJBasePacker& operator=(const HJBasePacker&) = delete;
    HJBasePacker(HJBasePacker&&) = default;
    HJBasePacker& operator=(HJBasePacker&&) = default;

    virtual HJVibeData::Ptr pack(const HJVibeData::Ptr& data) = 0;
    virtual HJVibeData::Ptr unpack(const HJVibeData::Ptr& data) = 0;
protected:
    HJParams::Ptr m_params{};
};

NS_HJ_END