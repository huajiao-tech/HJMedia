//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <type_traits>
#include <cstddef>
#include "HJCommons.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJDefaultValue {
public:
    HJDefaultValue() = delete;
    HJDefaultValue(const HJDefaultValue&) = delete;
    HJDefaultValue& operator=(const HJDefaultValue&) = delete;

    template<typename T>
    static constexpr typename std::enable_if<std::is_void<T>::value, void>::type
        get() noexcept {}

    template<typename T>
    static constexpr typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type
        get() noexcept {
        return nullptr;
    }

    template<typename T>
    static constexpr typename std::enable_if<std::is_pointer<T>::value, T>::type
        get() noexcept {
        return nullptr;
    }

    template<typename T>
    static constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
        get() noexcept {
        return static_cast<T>(0);
    }

    template<typename T>
    static constexpr typename std::enable_if<std::is_enum<T>::value, T>::type
        get() noexcept {
        return static_cast<T>(0);
    }

    template<typename T>
    static constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
        get() noexcept {
        return static_cast<T>(0.0);
    }

    template<typename T>
    static typename std::enable_if<
        !std::is_void<T>::value &&
        !std::is_pointer<T>::value &&
        !std::is_integral<T>::value &&
        !std::is_enum<T>::value &&
        !std::is_floating_point<T>::value &&
        !std::is_same<T, std::nullptr_t>::value,
        T>::type
        get() noexcept(std::is_nothrow_default_constructible<T>::value) {
        return T();
    }

    template<typename T>
    static constexpr bool is_supported() noexcept {
        return true;
    }

    template<typename T>
    static constexpr bool is_default_constructible() noexcept {
        return std::is_default_constructible<T>::value;
    }

    //template<typename T>
    //static typename std::enable_if<std::is_void<T>::value, void>::type
    //defaultValue() {}

    //template<typename T>
    //static typename std::enable_if<std::is_pointer<T>::value, T>::type
    //defaultValue() {
    //    return nullptr;
    //}

    //template<typename T>
    //static typename std::enable_if<std::is_integral<T>::value, T>::type
    //defaultValue() {
    //    return 0;
    //}
    //template<typename T>
    //static typename std::enable_if<std::is_floating_point<T>::value, T>::type
    //    defaultValue() {
    //    return 0.0;
    //}
    //template<typename T>
    //static typename std::enable_if<!std::is_void<T>::value && 
    //    !std::is_pointer<T>::value &&
    //    !std::is_integral<T>::value &&
    //    !std::is_floating_point<T>::value,
    //    T>::type
    //defaultValue() {
    //    return T();
    //}
};


NS_HJ_END