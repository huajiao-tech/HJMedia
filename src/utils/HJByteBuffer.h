//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <stdexcept>
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
    HJByteBuffer(size_t capacity = 1024)
        : m_buffer(capacity) {
    }

    const int IS_LITTLE_ENDIAN() {
        int __dummy = 1;
        return (*((unsigned char*)(&(__dummy))));
    }

    template<typename T>
    void write(T value) {
        static_assert(std::is_integral_v<T>, "Integral required.");
        ensureCapacity(sizeof(T));
        T networkValue = swapIfNeeded(value);
        memcpy(&m_buffer[m_writePos], &networkValue, sizeof(T));
        m_writePos += sizeof(T);
    }

    void write24(uint32_t value) {
        ensureCapacity(3);
        if (IS_LITTLE_ENDIAN()) {
            uint8_t buf[3];
            buf[0] = (value >> 16) & 0xFF;
            buf[1] = (value >> 8) & 0xFF;
            buf[2] = value & 0xFF;
            memcpy(&m_buffer[m_writePos], buf, 3);
            //m_buffer[m_writePos++] = (value >> 16) & 0xFF;
            //m_buffer[m_writePos++] = (value >> 8) & 0xFF;
            //m_buffer[m_writePos++] = value & 0xFF;
        } else {
            uint8_t* p = reinterpret_cast<uint8_t*>(&value);
            memcpy(&m_buffer[m_writePos], p + 1, 3);
        }
        m_writePos += 3;
    }

    void writeBytes(const uint8_t* data, size_t length) {
        if (!data || length == 0) return;
        ensureCapacity(length);
        memcpy(&m_buffer[m_writePos], data, length);
        m_writePos += length;
    }

    template<typename T>
    T read() {
        static_assert(std::is_integral_v<T>, "Integral required.");
        checkAvailable(sizeof(T));
        T value;
        memcpy(&value, &m_buffer[m_readPos], sizeof(T));
        m_readPos += sizeof(T);
        return swapIfNeeded(value);
    }

    uint32_t read24() {
        checkAvailable(3);
        uint32_t value = 0;
        if(IS_LITTLE_ENDIAN()) {
            value = (static_cast<uint32_t>(m_buffer[m_readPos]) << 16) |
                (static_cast<uint32_t>(m_buffer[m_readPos + 1]) << 8) |
                static_cast<uint32_t>(m_buffer[m_readPos + 2]);
        }
        else {
            value = (static_cast<uint32_t>(m_buffer[m_readPos + 2]) << 16) |
                (static_cast<uint32_t>(m_buffer[m_readPos + 1]) << 8) |
                static_cast<uint32_t>(m_buffer[m_readPos]);
        }
        m_readPos += 3;
        return value;
    }

    void readBytes(uint8_t* dest, size_t length) {
        checkAvailable(length);
        memcpy(dest, &m_buffer[m_readPos], length);
        m_readPos += length;
    }

    const uint8_t* data() const { return m_buffer.data(); }
    size_t size() const { return m_writePos; }
    size_t capacity() const { return m_buffer.capacity(); }
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
        if (m_writePos + additional > m_buffer.size()) {
            m_buffer.resize(HJ_MAX(m_buffer.size() * 2, m_writePos + additional));
        }
    }

    void checkAvailable(size_t length) const {
        if (m_readPos + length > m_writePos) {
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
