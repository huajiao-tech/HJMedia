//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <vector>
#include <cstdint>
#include <tuple>
#include <utility>
#include <type_traits>
#include <shared_mutex>
#include "HJUtilitys.h"
#include "HJBasePacker.h"
#include "HJLZ4Packer.h"
#include "HJZlibPacker.h"
#include "HJSegPacker.h"
#include "HJException.h"
#include "HJUUIDTools.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJPackerManager : public HJBasePacker {
public:
    HJ_DECLARE_PUWTR(HJPackerManager);

    HJPackerManager(const HJParams::Ptr& params = nullptr) : HJBasePacker(params) {}
    
    HJPackerManager(const HJPackerManager&) = delete;
    HJPackerManager& operator=(const HJPackerManager&) = delete;
    HJPackerManager(HJPackerManager&&) = default;
    HJPackerManager& operator=(HJPackerManager&&) = default;

    virtual HJVibeData::Ptr pack(const HJVibeData::Ptr& data) override;
    virtual HJVibeData::Ptr unpack(const HJVibeData::Ptr& data) override;

    template <typename... Packers>
    void registerPackers() {
        std::unique_lock lock(m_packersMutex);
        (registerSinglePacker<Packers>(), ...);
    }
    template <typename... PackerPairs>
    void registerPackers(PackerPairs&&... pairs) {
        std::unique_lock lock(m_packersMutex);
        (registerSinglePackerWithArgs<typename std::decay_t<PackerPairs>::first_type>(
            std::forward<PackerPairs>(pairs).second
        ), ...);
    }
private:
    template <typename PackerType>
    void registerSinglePacker() {
        try
        {
            auto creator = [](const HJParams::Ptr& params) -> HJBasePacker::Ptr {
                return std::make_shared<PackerType>(params);
            };
            // m_packerCreators.push_back(creator);
            auto packer = creator(m_params);
            if (packer) {
                //HJFLogi("create packer success, name: {}", packer->getName());
                m_packers.push_back(std::move(packer));
            }
        } catch(const HJException& e) {
            HJ_EXCEPT(HJException::ERR_INVALID_CALL, "create packer failed");
        }
    }
    template <typename PackerType, typename Tuple>
    void registerSinglePackerWithArgs(Tuple&& args) {
        try
        {        
            using CapturedTuple = std::decay_t<Tuple>;
            auto creator = [capturedArgs = CapturedTuple(std::forward<Tuple>(args))](const HJParams::Ptr& params) mutable -> HJBasePacker::Ptr {
                return std::apply([&](auto&&... unpackedArgs) {
                    return std::make_shared<PackerType>(params, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                }, std::move(capturedArgs));
            };
            // m_packerCreators.push_back(creator);
            auto packer = creator(m_params);
            if (packer) {
                //HJFLogi("create packer success, name: {}", packer->getName());
                m_packers.push_back(std::move(packer));
            }
        } catch(const HJException& e) {
            HJ_EXCEPT(HJException::ERR_INVALID_CALL, "create packer failed");
        }
    }
public:
    static bool writeHeader(const HJXVibHeader& h, std::vector<uint8_t>& out);
    static bool readHeader(const uint8_t* p, size_t n, HJXVibHeader& h);
public:
    static constexpr uint8_t MAX_SEG_COUNT = 255;
private:
    // std::vector<std::function<HJBasePacker::Ptr(const HJParams::Ptr& params)>> m_packerCreators;
    std::vector<HJBasePacker::Ptr> m_packers;
    mutable std::shared_mutex m_packersMutex;
};

NS_HJ_END