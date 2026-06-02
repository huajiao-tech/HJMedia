//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <cstdint>
#include <vector>
#include <limits>
#include <memory>
#include <type_traits>
#include <algorithm>
#include "HJMacros.h"
#include "HJException.h"

// #if defined(HJ_OS_WINDOWS) || defined(HJ_OS_ANDROID) || defined(HJ_OS_HARMONY) || defined(__LITTLE_ENDIAN__)
// #define IS_LITTLE_ENDIAN 1
// #elif defined(__BIG_ENDIAN__)
// #define IS_LITTLE_ENDIAN 0
// #else
// #error "Unknown endianness, please define IS_LITTLE_ENDIAN manually."
// #endif

//#ifdef __ARM_FEATURE_ENDIAN
//    #if __ARM_FEATURE_ENDIAN & 1
//        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
//            #define IS_LITTLE_ENDIAN 1
//        #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
//            #define IS_LITTLE_ENDIAN 0
//        #else
//            #error "Unsupported endianness"
//        #endif
//    #else
//        #error "ARM endianness not supported"
//    #endif
//#elif defined(__BYTE_ORDER__)
//    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
//        #define IS_LITTLE_ENDIAN 1
//    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
//        #define IS_LITTLE_ENDIAN 0
//    #else
//        #error "Unsupported endianness"
//    #endif
//#else
//    #include <cstdint>
//    // namespace {
//    //     constexpr bool detectEndianness() {
//    //         const std::uint32_t testValue = 0x01020304;
//    //         const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&testValue);
//    //         return bytes[0] == 0x04;
//    //     }
//    // }
//    #define IS_LITTLE_ENDIAN 1 //detectEndianness()
//#endif

#if defined(HJ_OS_WINDOWS)
#include <stdlib.h>
//#define IS_LITTLE_ENDIAN 1
//#define IS_LITTLE_ENDIAN (*(uint16_t*)"\0\xff" == 0xff00)
#define htobe16(x) _byteswap_ushort(x)
#define htobe32(x) _byteswap_ulong(x)
#define htobe64(x) _byteswap_uint64(x)
#else
// #include <endian.h>
// #define IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#define htobe16(x) __builtin_bswap16(x)
#define htobe32(x) __builtin_bswap32(x)
#define htobe64(x) __builtin_bswap64(x)
#endif

NS_HJ_BEGIN
//***********************************************************************************//
class HJByteBuffer {
public:
    using Ptr = std::shared_ptr<HJByteBuffer>;
    static constexpr uint32_t kMax24BitValue = 0x00FFFFFFU;

    HJByteBuffer(size_t capacity = 1024)
        : m_buffer(capacity) {
    }

    static bool IS_LITTLE_ENDIAN() noexcept {
        const uint16_t dummy = 0x1;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&dummy);
        return bytes[0] == 0x1;
    }

    template<typename T>
    void write(T value) {
        static_assert(std::is_integral_v<T>, "Integral required.");
        ensureCapacity(sizeof(T));
        T networkValue = swapIfNeeded(value);
        const auto* byte_data = reinterpret_cast<const uint8_t*>(&networkValue);
        std::copy_n(byte_data, sizeof(T), m_buffer.data() + m_writePos);
        m_writePos += sizeof(T);
    }

    void write24(uint32_t value) {
        if (value > kMax24BitValue) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "24-bit value out of range");
        }
        ensureCapacity(3);
        m_buffer[m_writePos] = static_cast<uint8_t>((value >> 16) & 0xFFU);
        m_buffer[m_writePos + 1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
        m_buffer[m_writePos + 2] = static_cast<uint8_t>(value & 0xFFU);
        m_writePos += 3;
    }

    void writeBytes(const uint8_t* data, size_t length) {
        if (length == 0) {
            return;
        }
        if (data == nullptr) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "Buffer write data is null");
        }
        ensureCapacity(length);
        std::copy_n(data, length, m_buffer.data() + m_writePos);
        m_writePos += length;
    }

    template<typename T>
    T read() {
        static_assert(std::is_integral_v<T>, "Integral required.");
        checkAvailable(sizeof(T));
        T value;
        auto* byte_data = reinterpret_cast<uint8_t*>(&value);
        std::copy_n(m_buffer.data() + m_readPos, sizeof(T), byte_data);
        m_readPos += sizeof(T);
        return swapIfNeeded(value);
    }

    uint32_t read24() {
        checkAvailable(3);
        const uint32_t value = (static_cast<uint32_t>(m_buffer[m_readPos]) << 16) |
            (static_cast<uint32_t>(m_buffer[m_readPos + 1]) << 8) |
            static_cast<uint32_t>(m_buffer[m_readPos + 2]);
        m_readPos += 3;
        return value;
    }

    void readBytes(uint8_t* dest, size_t length) {
        if (length == 0) {
            return;
        }
        if (dest == nullptr) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "Buffer read destination is null");
        }
        checkAvailable(length);
        std::copy_n(m_buffer.data() + m_readPos, length, dest);
        m_readPos += length;
    }

    const uint8_t* data() const { return m_buffer.data(); }
    size_t size() const { return m_writePos; }
    size_t capacity() const { return m_buffer.size(); }
    size_t remaining() const { return m_writePos - m_readPos; }
    bool eof() const { return m_readPos >= m_writePos; }
    void clear() { m_readPos = m_writePos = 0; }
    void reset() { m_readPos = 0; }

private:
    template<typename T>
    T swapIfNeeded(T value) const {
        if (!IS_LITTLE_ENDIAN()) {
            return value; // Big-endian system, no swap needed for network order
        }
        if constexpr (std::is_same_v<T, uint16_t>) {
            return htobe16(value);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return htobe32(value);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return htobe64(value);
        } if constexpr (std::is_same_v<T, int16_t>) {
            return htobe16(value);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return htobe32(value);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return htobe64(value);
        } else {
            return value; // For types like uint8_t, no swap is needed
        }
        return value;
    }

    void ensureCapacity(size_t additional) {
        if (additional > std::numeric_limits<size_t>::max() - m_writePos) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "Buffer size overflow");
        }

        const size_t required_size = m_writePos + additional;
        if (required_size > m_buffer.size()) {
            const size_t current_size = m_buffer.size();
            const size_t doubled_size = current_size > (std::numeric_limits<size_t>::max() / 2) ?
                std::numeric_limits<size_t>::max() :
                current_size * 2;
            const size_t new_size = std::max(required_size, std::max<size_t>(1, doubled_size));
            m_buffer.resize(new_size);
        }
    }

    void checkAvailable(size_t length) const {
        if (length > remaining()) {
            HJ_EXCEPT(HJException::ERR_INVALID_CALL, "Buffer read out of range");
        }
    }

    //static constexpr bool isLittleEndian() {
    //    return *reinterpret_cast<const uint16_t*>("\1\0") == 1;
    //}
    //static constexpr bool littleEndian = (*(const uint16_t*)("\1\0") == 1);

private:
    std::vector<uint8_t> m_buffer;
    size_t               m_readPos{ 0 };
    size_t               m_writePos{ 0 };
};

NS_HJ_END
