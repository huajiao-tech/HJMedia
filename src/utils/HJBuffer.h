//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <functional>
#include "HJMacros.h"
#include "HJObject.h"

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_ALIGNED_DEFAULT     32
template <typename T>
struct HJAlignedAllocator {
    typedef T value_type;

    HJAlignedAllocator() = default;

    template <class U>
    HJAlignedAllocator(const HJAlignedAllocator<U>&) {}

    T* allocate(std::size_t n) {
        void* ptr = ::operator new(n * sizeof(T), std::align_val_t(HJ_ALIGNED_DEFAULT));
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p) {
        ::operator delete(p, std::align_val_t(HJ_ALIGNED_DEFAULT));
    }
};
//std::allocator<int, HJAlignedAllocator<int>> alloc;

//***********************************************************************************//
template <typename T, size_t Alignment = alignof(T)>
class HJFlexibleAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    static constexpr size_t alignment = Alignment;

    template <typename U>
    struct rebind {
        using other = HJFlexibleAllocator<U, Alignment>;
    };

    HJFlexibleAllocator() = default;

    template <typename U>
    HJFlexibleAllocator(const HJFlexibleAllocator<U, Alignment>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        //if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        //    throw std::bad_array_new_length();
        if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
            throw std::bad_array_new_length();

        if constexpr (Alignment <= alignof(std::max_align_t)) {
            void* ptr = ::operator new(n * sizeof(T));
            if (!ptr) throw std::bad_alloc();
            return static_cast<T*>(ptr);
        }
        else {
            void* ptr = ::operator new(n * sizeof(T), std::align_val_t(Alignment));
            if (!ptr) throw std::bad_alloc();
            return static_cast<T*>(ptr);
        }
    }

    void deallocate(T* p, std::size_t n) noexcept {
        if constexpr (Alignment <= alignof(std::max_align_t)) {
            ::operator delete(p);
        }
        else {
            ::operator delete(p, std::align_val_t(Alignment));
        }
    }

    bool operator==(const HJFlexibleAllocator&) const noexcept { return true; }
    bool operator!=(const HJFlexibleAllocator&) const noexcept { return false; }
};
using HJDefaultAllocator = HJFlexibleAllocator<uint8_t>;
using HJAligned16Allocator = HJFlexibleAllocator<uint8_t, 16>;
using HJAligned64Allocator = HJFlexibleAllocator<uint8_t, 64>;
//std::vector<uint8_t, HJDefaultAllocator> vec1;
//std::vector<uint8_t, HJAligned16Allocator> vec2;
//std::vector<uint8_t, HJAligned64Allocator> vec3;
//std::vector<float, HJFlexibleAllocator<float, 64>> vec3;

class HJBuffer : public virtual HJObject
{
public:
    using Ptr = std::shared_ptr<HJBuffer>;
    HJBuffer() = default;
    HJBuffer(const HJBuffer::Ptr& other);
    HJBuffer(size_t capacity, size_t offset = 0);
    HJBuffer(const uint8_t* data, size_t size);
    virtual ~HJBuffer();
    
    uint8_t* data() const {
        return m_data + m_offset;
    }
    size_t size() const {
        return m_size - m_offset;
    }
    uint8_t* spaceData() {
        return m_data + m_size;
    }
    size_t space() const {
        return m_capacity - m_size;
    }
    void setSize(size_t size) {
        if (m_capacity < size) {
            setCapacity(size);
        }
        m_size = size;
    }
    void addSize(int64_t offset) {
        setSize(m_size + offset);
    }
    size_t offset() const {
        return m_offset;
    }
    void setOffset(size_t offset) {
        if (offset <= m_size && offset <= m_capacity) {
            m_offset = offset;
        }
    }
    size_t capacity() const {
        return m_capacity;
    }
    void setCapacity(size_t size) {
        realloc(size);
    }
    void reset() {
//        m_capacity = 0;
        m_size = 0;
        m_offset = 0;
    }
    void resetOffset() {
        m_offset = 0;
    }
    void setData(const uint8_t* data, size_t size);
    void appendData(const uint8_t* data, size_t size);
    void appendData(const HJBuffer::Ptr& block) {
        if (block) {
            appendData(block->data(), block->size());
        }
    }
    size_t read(const uint8_t* data, size_t size) {
        if (!data || size <= 0) {
            return 0;
        }
        if (m_offset + size > m_size) {
            size = m_size - m_offset;
        }
        memcpy((void *)data, this->data(), size);
        m_offset += size;

        return size;
    }
    //
    void write(const uint8_t* data, size_t size) {
        if (data && size > 0) {
            appendData(data, size);
        }
    }
    void wstring(const std::string str) {
        const size_t len = str.length();
        wb16((uint16_t)len);
        write((uint8_t*)str.c_str(), len);
    }
    void w8(uint8_t u8) {
        write(&u8, sizeof(uint8_t));
    }
    void wl16(uint16_t u16) {
        w8((uint8_t)u16);
        w8(u16 >> 8);
    }
    void wl24(uint32_t u24) {
        w8((uint8_t)u24);
        wl16((uint16_t)(u24 >> 8));
    }

    void wl32(uint32_t u32) {
        wl16((uint16_t)u32);
        wl16((uint16_t)(u32 >> 16));
    }

    void wl64(uint64_t u64) {
        wl32((uint32_t)u64);
        wl32((uint32_t)(u64 >> 32));
    }

    void wlf(float f) {
        wl32(*(uint32_t*)&f);
    }

    void wld(double d) {
        wl64(*(uint64_t*)&d);
    }

    void wb16(uint16_t u16) {
        w8(u16 >> 8);
        w8((uint8_t)u16);
    }

    void wb24(uint32_t u24) {
        wb16((uint16_t)(u24 >> 8));
        w8((uint8_t)u24);
    }

    void wb32(uint32_t u32) {
        wb16((uint16_t)(u32 >> 16));
        wb16((uint16_t)u32);
    }

    void wb64(uint64_t u64) {
        wb32((uint32_t)(u64 >> 32));
        wb32((uint32_t)u64);
    }

    void wbf(float f) {
        wb32(*(uint32_t*)&f);
    }

    void wbd(double d) {
        wb64(*(uint64_t*)&d);
    }
    void wtimestamp(int32_t i32) {
        wb24((uint32_t)(i32 & 0xFFFFFF));
        w8((uint32_t)(i32 >> 24) & 0x7F);
    }
    //
    void setTimestamp(const int64_t time) {
        m_timestamp = time;
    }
    const int64_t getTimestamp() const {
        return m_timestamp;
    }
private:
    uint8_t* realloc(size_t capacity, bool copy = false)
    {
        if (capacity <= m_capacity) {
            return m_data;
        }
        uint8_t* buf = m_alloc.allocate(capacity);
        if (m_data)
        {
            if(m_size > 0 && copy) {
                memcpy(buf, m_data, m_size);
            }
            m_alloc.deallocate(m_data, m_capacity);
        }
        m_data = buf;
        m_capacity = capacity;
        
        return m_data;
    }
    //
    uint8_t*                m_data = NULL;
    size_t                  m_size = 0;
    size_t                  m_offset = 0;
    size_t                  m_capacity = 0;
    std::allocator<uint8_t> m_alloc;
    int64_t                 m_timestamp = 0;
};

//***********************************************************************************//
#if !defined(HJ_OS_LINUX)
/**
 * int* p3 = a.aligned_alloc<int>(32);
 */
template <std::size_t N>
struct HJAllocator
{
    char data[N];
    void* p;
    std::size_t sz;
    HJAllocator() : p(data), sz(N) {}
    template <typename T>
    T* alignedAlloc(std::size_t a = alignof(T))
    {
        if (std::align(a, sizeof(T), p, sz))
        {
            T* result = reinterpret_cast<T*>(p);
            p = (char*)p + sizeof(T);
            sz -= sizeof(T);
            return result;
        }
        return nullptr;
    }
};
#endif

//std::vector<int, std::allocator<int>> vec;
//#include <memory_resource>
//std::vector<float, std::aligned_allocator<float, 64>> alignedVec;
//std::pmr::vector<int> vec{ std::pmr::get_default_resource() };

//***********************************************************************************//
template<typename T>
class HJAudioBuffers : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAudioBuffers<T>>;
    HJAudioBuffers(int capacity, int channel)
        : m_capacity(capacity)
        , m_channel(channel)
    {
        for (size_t i = 0; i < channel; i++)
        {
            T* p = new T[capacity];
            m_datas.emplace_back(p);
        }
    }
    virtual ~HJAudioBuffers() {
        for (T* p : m_datas) {
            delete p;
        }
        m_datas.clear();
    }
    T** data() {
        return m_datas.data();
    }
    int getsamples() {
        return m_samples;
    }
    void setsamples(int samples) {
        m_samples = samples;
    }
private:
    int m_capacity{0};
    int m_channel{0};
    std::vector<T*> m_datas;
    int m_samples{0};
};
using HJAudioFBuffers = HJAudioBuffers<float>;
using HJAudioIBuffers = HJAudioBuffers<int16_t>;

NS_HJ_END
