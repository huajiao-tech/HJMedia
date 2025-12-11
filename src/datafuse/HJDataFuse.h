//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <cstdint>
#include "HJUtilitys.h"

NS_HJ_BEGIN
class HJPackerManager;
//***********************************************************************************//
class HJDataFuse : public HJObject {
public:
    HJ_DECLARE_PUWTR(HJDataFuse);
    HJDataFuse(const HJParams::Ptr& params = nullptr);
    virtual ~HJDataFuse();

    HJDataFuse(const HJDataFuse&) = delete;
    HJDataFuse& operator=(const HJDataFuse&) = delete;
    HJDataFuse(HJDataFuse&&) = default;
    HJDataFuse& operator=(HJDataFuse&&) = default;

    std::vector<HJRawBuffer> pack(const HJRawBuffer& data);
    std::vector<HJRawBuffer> pack(const std::string& data);
    HJRawBuffer unpack(const std::vector<HJRawBuffer>& data);
protected:
    HJParams::Ptr m_params{};
    std::unique_ptr<HJPackerManager> m_packerManager{};
};

NS_HJ_END