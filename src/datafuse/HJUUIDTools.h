//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include <array>
#include <iostream>
#include <iomanip>
#include <optional>
#include <cctype>

NS_HJ_BEGIN
//***********************************************************************************//
struct HJUUID {
    std::array<uint8_t, 16> data{};
    //
    std::string toString(bool separator = false) const {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for(size_t i = 0; i < data.size(); ++i) {
            ss << std::setw(2) << static_cast<int>(data[i]);
            if (separator) {
                if (i == 3 || i == 5 || i == 7 || i == 9) {
                    ss << "-";
                }
            }
        }
        return ss.str();
    }

    bool fromString(const std::string& str) {
        std::array<uint8_t, 16> parsed{};
        std::string hex_only;
        hex_only.reserve(32);

        // Remove separators and validate characters.
        for (char ch : str) {
            if (ch == '-') {
                continue;
            }
            if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                return false;
            }
            hex_only.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (hex_only.size() != 32) {
            return false;
        }

        auto hex_to_byte = [](char high, char low, uint8_t& out) -> bool {
            auto nibble = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
                return -1;
            };
            int h = nibble(high);
            int l = nibble(low);
            if (h < 0 || l < 0) return false;
            out = static_cast<uint8_t>((h << 4) | l);
            return true;
        };

        for (size_t i = 0, di = 0; i < hex_only.size(); i += 2, ++di) {
            if (!hex_to_byte(hex_only[i], hex_only[i + 1], parsed[di])) {
                return false;
            }
        }

        data = parsed;
        return true;
    }
};

class HJUUIDTools : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJUUIDTools);

    static std::optional<HJUUID> generate_uuid();
    static HJUUID derive_uuid(const HJUUID& parent, std::optional<int64_t> number = std::nullopt);
    static bool verify_uuid(const HJUUID& child, const HJUUID& parent, std::optional<int64_t> number = std::nullopt);
    static HJUUID derive_uuid(const HJUUID& parent, const std::vector<uint8_t>& data);
    static bool verify_uuid(const HJUUID& child, const HJUUID& parent, const std::vector<uint8_t>& data);
    //
    static HJUUID derive_uuid(const std::string& parent, std::optional<int64_t> number = std::nullopt);
    static bool verify_uuid(const std::string& child, const std::string& parent, std::optional<int64_t> number = std::nullopt);
    //
    static HJUUID derive_uuid_from_main(std::optional<int64_t> number = std::nullopt);
    static bool verify_uuid_from_main(const std::string& child, std::optional<int64_t> number = std::nullopt);
public:
    static const HJUUID HJ_MAIN_UUID;
    static const HJUUID HJ_DERIVE_UUID0;  //derive number = 0
};

NS_HJ_END
