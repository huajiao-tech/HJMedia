#pragma once

#include "refl.hpp"
#include "HJMediaUtils.h"
//#include <string_view>
//#include <utility>

#define REFL_FIELD_WRAP_COMMA(x) , field(x)
#define REFL_AUTO_SIMPLE(Type, ...) \
    REFL_AUTO(type(Type) REFL_DETAIL_FOR_EACH(REFL_FIELD_WRAP_COMMA, __VA_ARGS__))

// Enum value <-> string helpers.
// Usage:
//   #define ORIENTATION_ENUM(M, EnumType) \
//       M(EnumType, Horizontal) \
//       M(EnumType, Vertical)
//   enum class Orientation { ORIENTATION_ENUM(HJ_ENUM_DECLARE_ITEM, Orientation) };
//   HJ_ENUM_STRING(Orientation, ORIENTATION_ENUM)
#define HJ_ENUM_DECLARE_ITEM(EnumType, Value) Value,
#define HJ_ENUM_PAIR(EnumType, Value) { EnumType::Value, #Value },
#define HJ_ENUM_STRING(EnumType, ListMacro) \
    inline constexpr std::pair<EnumType, std::string_view> EnumType##_enum_map[] = { \
        ListMacro(HJ_ENUM_PAIR, EnumType) \
    }; \
    inline constexpr std::string_view EnumType##_to_string(EnumType value) { \
        for (auto&& item : EnumType##_enum_map) { \
            if (item.first == value) { \
                return item.second; \
            } \
        } \
        return {}; \
    } \
    inline bool EnumType##_from_string(std::string_view text, EnumType& out) { \
        for (auto&& item : EnumType##_enum_map) { \
            if (item.second == text) { \
                out = item.first; \
                return true; \
            } \
        } \
        return false; \
    }


REFL_TEMPLATE((typename T), (HJ::HJPoint<T>))
REFL_FIELD(x)
REFL_FIELD(y)
REFL_END

REFL_TEMPLATE((typename T), (HJ::HJRect<T>))
REFL_FIELD(x)
REFL_FIELD(y)
REFL_FIELD(w)
REFL_FIELD(h)
REFL_END

REFL_TEMPLATE((typename T), (HJ::HJVec4<T>))
REFL_FIELD(x)
REFL_FIELD(y)
REFL_FIELD(z)
REFL_FIELD(w)
REFL_END

REFL_TEMPLATE((typename T), (HJ::HJRange<T>))
REFL_FIELD(begin)
REFL_FIELD(end)
REFL_END
