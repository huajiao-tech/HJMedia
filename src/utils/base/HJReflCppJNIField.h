#pragma once


#include "HJComUtils.h"
#include "HJReflCpp.h"
#include "HJError.h"
#include "HJJNIField.h"
#include <variant>
#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

NS_HJ_BEGIN

#if defined(ANDROID_LIB)

namespace refljni_detail {

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {
    using value_type = T;
};

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
    using value_type = T;
};

template <typename T>
struct is_string : std::is_same<std::decay_t<T>, std::string> {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
inline constexpr bool is_string_v = is_string<T>::value;

template <typename T>
using decay_t = std::decay_t<T>;

inline jobject get_public_field_object(JNIEnv* env, jobject obj, const std::string& name, bool& o_found)
{
    o_found = false;
    if (!env || !obj) {
        return nullptr;
    }
    jclass clsObj = env->GetObjectClass(obj);
    if (!clsObj) {
        return nullptr;
    }
    jclass clsClass = env->FindClass("java/lang/Class");
    jclass clsField = env->FindClass("java/lang/reflect/Field");
    if (!clsClass || !clsField) {
        env->DeleteLocalRef(clsObj);
        if (clsClass) {
            env->DeleteLocalRef(clsClass);
        }
        if (clsField) {
            env->DeleteLocalRef(clsField);
        }
        return nullptr;
    }
    jmethodID midGetField = env->GetMethodID(clsClass, "getField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jmethodID midFieldGet = env->GetMethodID(clsField, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    jobject valueObj = nullptr;
    if (midGetField && midFieldGet) {
        jstring jname = env->NewStringUTF(name.c_str());
        jobject fieldObj = env->CallObjectMethod(clsObj, midGetField, jname);
        env->DeleteLocalRef(jname);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            fieldObj = nullptr;
        }
        if (fieldObj) {
            valueObj = env->CallObjectMethod(fieldObj, midFieldGet, obj);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                valueObj = nullptr;
            }
            env->DeleteLocalRef(fieldObj);
            o_found = true;
        }
    }
    env->DeleteLocalRef(clsObj);
    env->DeleteLocalRef(clsClass);
    env->DeleteLocalRef(clsField);
    return valueObj;
}

inline bool is_java_array(JNIEnv* env, jobject obj)
{
    if (!env || !obj) {
        return false;
    }
    jclass clsClass = env->FindClass("java/lang/Class");
    if (!clsClass) {
        return false;
    }
    jmethodID midIsArray = env->GetMethodID(clsClass, "isArray", "()Z");
    bool isArray = false;
    if (midIsArray) {
        jclass clsObj = env->GetObjectClass(obj);
        if (clsObj) {
            isArray = env->CallBooleanMethod(clsObj, midIsArray);
            env->DeleteLocalRef(clsObj);
        }
    }
    env->DeleteLocalRef(clsClass);
    return isArray;
}

inline bool is_java_list(JNIEnv* env, jobject obj)
{
    if (!env || !obj) {
        return false;
    }
    jclass clsList = env->FindClass("java/util/List");
    if (!clsList) {
        return false;
    }
    bool ret = env->IsInstanceOf(obj, clsList);
    env->DeleteLocalRef(clsList);
    return ret;
}

template <typename T>
int read_boxed_number(JNIEnv* env, jobject obj, T& value)
{
    if (!env || !obj) {
        return HJErrInvalidParams;
    }
    if constexpr (std::is_same_v<T, bool>) {
        jclass clsBool = env->FindClass("java/lang/Boolean");
        if (!clsBool || !env->IsInstanceOf(obj, clsBool)) {
            if (clsBool) {
                env->DeleteLocalRef(clsBool);
            }
            return HJErrNotSupport;
        }
        jmethodID mid = env->GetMethodID(clsBool, "booleanValue", "()Z");
        if (!mid) {
            env->DeleteLocalRef(clsBool);
            return HJErrNotSupport;
        }
        value = static_cast<bool>(env->CallBooleanMethod(obj, mid));
        env->DeleteLocalRef(clsBool);
        return HJ_OK;
    } else {
        jclass clsNum = env->FindClass("java/lang/Number");
        if (!clsNum || !env->IsInstanceOf(obj, clsNum)) {
            if (clsNum) {
                env->DeleteLocalRef(clsNum);
            }
            return HJErrNotSupport;
        }
        if constexpr (std::is_same_v<T, int>) {
            jmethodID mid = env->GetMethodID(clsNum, "intValue", "()I");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<int>(env->CallIntMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, short>) {
            jmethodID mid = env->GetMethodID(clsNum, "shortValue", "()S");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<short>(env->CallShortMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, int64_t>) {
            jmethodID mid = env->GetMethodID(clsNum, "longValue", "()J");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<int64_t>(env->CallLongMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            jmethodID mid = env->GetMethodID(clsNum, "longValue", "()J");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<uint64_t>(env->CallLongMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, float>) {
            jmethodID mid = env->GetMethodID(clsNum, "floatValue", "()F");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<float>(env->CallFloatMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, double>) {
            jmethodID mid = env->GetMethodID(clsNum, "doubleValue", "()D");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<double>(env->CallDoubleMethod(obj, mid));
        } else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>) {
            jmethodID mid = env->GetMethodID(clsNum, "intValue", "()I");
            if (!mid) {
                env->DeleteLocalRef(clsNum);
                return HJErrNotSupport;
            }
            value = static_cast<T>(env->CallIntMethod(obj, mid));
        } else {
            env->DeleteLocalRef(clsNum);
            return HJErrNotSupport;
        }
        env->DeleteLocalRef(clsNum);
        return HJ_OK;
    }
}

inline int read_string(JNIEnv* env, jobject obj, std::string& value)
{
    if (!env || !obj) {
        return HJErrInvalidParams;
    }
    jclass clsString = env->FindClass("java/lang/String");
    if (!clsString) {
        return HJErrNotSupport;
    }
    bool isStr = env->IsInstanceOf(obj, clsString);
    env->DeleteLocalRef(clsString);
    if (!isStr) {
        return HJErrNotSupport;
    }
    jstring jstr = static_cast<jstring>(obj);
    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
    if (!cstr) {
        return HJErrInvalidParams;
    }
    value = cstr;
    env->ReleaseStringUTFChars(jstr, cstr);
    return HJ_OK;
}

template <typename T>
int deserialize_object(JNIEnv* env, jobject obj, T& value);

template <typename Elem>
int read_element(JNIEnv* env, jobject elemObj, Elem& value)
{
    if constexpr (is_string_v<Elem>) {
        return read_string(env, elemObj, value);
    } else if constexpr (std::is_enum_v<Elem>) {
        using Underlying = std::underlying_type_t<Elem>;
        Underlying raw{};
        int ret = read_boxed_number(env, elemObj, raw);
        if (ret < 0) {
            return ret;
        }
        value = static_cast<Elem>(raw);
        return HJ_OK;
    } else if constexpr (std::is_same_v<Elem, bool> ||
                         std::is_same_v<Elem, int> ||
                         std::is_same_v<Elem, short> ||
                         std::is_same_v<Elem, int64_t> ||
                         std::is_same_v<Elem, uint64_t> ||
                         std::is_same_v<Elem, unsigned int> ||
                         std::is_same_v<Elem, uint32_t> ||
                         std::is_same_v<Elem, float> ||
                         std::is_same_v<Elem, double>) {
        return read_boxed_number(env, elemObj, value);
    } else if constexpr (refl::trait::is_reflectable_v<Elem>) {
        return deserialize_object(env, elemObj, value);
    } else {
        return HJErrNotSupport;
    }
}

template <typename T>
int read_array_value(JNIEnv* env, jobject fieldObj, T& value)
{
    if (!fieldObj) {
        return HJErrNotExist;
    }
    if constexpr (!is_vector_v<T>) {
        return HJErrNotSupport;
    } else {
        using Elem = typename is_vector<T>::value_type;
        value.clear();

        if (is_java_array(env, fieldObj)) {
            jclass clsObjArray = env->FindClass("[Ljava/lang/Object;");
            if (!clsObjArray || !env->IsInstanceOf(fieldObj, clsObjArray)) {
                if (clsObjArray) {
                    env->DeleteLocalRef(clsObjArray);
                }
                return HJErrNotSupport;
            }
            if (clsObjArray) {
                env->DeleteLocalRef(clsObjArray);
            }
            auto arr = static_cast<jobjectArray>(fieldObj);
            jsize len = env->GetArrayLength(arr);
            value.reserve(static_cast<size_t>(len));
            for (jsize i = 0; i < len; i++) {
                jobject elemObj = env->GetObjectArrayElement(arr, i);
                if constexpr (is_shared_ptr_v<Elem>) {
                    using Pointee = typename is_shared_ptr<Elem>::value_type;
                    auto ptr = std::make_shared<Pointee>();
                    int ret = read_element(env, elemObj, *ptr);
                    if (ret < 0) {
                        env->DeleteLocalRef(elemObj);
                        return ret;
                    }
                    value.push_back(std::move(ptr));
                } else {
                    Elem elem{};
                    int ret = read_element(env, elemObj, elem);
                    if (ret < 0) {
                        env->DeleteLocalRef(elemObj);
                        return ret;
                    }
                    value.push_back(std::move(elem));
                }
                env->DeleteLocalRef(elemObj);
            }
            return HJ_OK;
        }

        if (is_java_list(env, fieldObj)) {
            jclass clsListObj = env->GetObjectClass(fieldObj);
            if (!clsListObj) {
                return HJErrInvalidParams;
            }
            jmethodID midSize = env->GetMethodID(clsListObj, "size", "()I");
            jmethodID midGet = env->GetMethodID(clsListObj, "get", "(I)Ljava/lang/Object;");
            if (!midSize || !midGet) {
                env->DeleteLocalRef(clsListObj);
                return HJErrNotSupport;
            }
            jint len = env->CallIntMethod(fieldObj, midSize);
            value.reserve(static_cast<size_t>(len));
            for (jint i = 0; i < len; i++) {
                jobject elemObj = env->CallObjectMethod(fieldObj, midGet, i);
                if constexpr (is_shared_ptr_v<Elem>) {
                    using Pointee = typename is_shared_ptr<Elem>::value_type;
                    auto ptr = std::make_shared<Pointee>();
                    int ret = read_element(env, elemObj, *ptr);
                    if (ret < 0) {
                        env->DeleteLocalRef(elemObj);
                        env->DeleteLocalRef(clsListObj);
                        return ret;
                    }
                    value.push_back(std::move(ptr));
                } else {
                    Elem elem{};
                    int ret = read_element(env, elemObj, elem);
                    if (ret < 0) {
                        env->DeleteLocalRef(elemObj);
                        env->DeleteLocalRef(clsListObj);
                        return ret;
                    }
                    value.push_back(std::move(elem));
                }
                env->DeleteLocalRef(elemObj);
            }
            env->DeleteLocalRef(clsListObj);
            return HJ_OK;
        }

        return HJErrNotSupport;
    }
}

template <typename T>
int read_value(JNIEnv* env, jobject obj, const std::string& key, T& value)
{
    if (!env || !obj) {
        return HJErrInvalidParams;
    }

    if constexpr (is_vector_v<T>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_array_value(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (is_string_v<T>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_string(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_enum_v<T>) {
        using Underlying = std::underlying_type_t<T>;
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        Underlying raw{};
        int ret = read_boxed_number(env, fieldObj, raw);
        env->DeleteLocalRef(fieldObj);
        if (ret < 0) {
            return ret;
        }
        value = static_cast<T>(raw);
        return HJ_OK;
    } else if constexpr (std::is_same_v<T, bool>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, int>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, short>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, uint32_t>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, float>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (std::is_same_v<T, double>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = read_boxed_number(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else if constexpr (is_shared_ptr_v<T>) {
        using Pointee = typename is_shared_ptr<T>::value_type;
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        auto ptr = std::make_shared<Pointee>();
        int ret = read_element(env, fieldObj, *ptr);
        env->DeleteLocalRef(fieldObj);
        if (ret < 0) {
            return ret;
        }
        value = std::move(ptr);
        return HJ_OK;
    } else if constexpr (refl::trait::is_reflectable_v<T>) {
        bool found = false;
        jobject fieldObj = get_public_field_object(env, obj, key, found);
        if (!found || !fieldObj) {
            return HJErrNotExist;
        }
        int ret = deserialize_object(env, fieldObj, value);
        env->DeleteLocalRef(fieldObj);
        return ret;
    } else {
        return HJErrNotSupport;
    }
}

template <typename T, size_t N>
int read_value(JNIEnv* env, jobject obj, const std::string& key, T (&value)[N])
{
    if (!env || !obj) {
        return HJErrInvalidParams;
    }
    bool found = false;
    jobject fieldObj = get_public_field_object(env, obj, key, found);
    if (!found || !fieldObj) {
        return HJErrNotExist;
    }
    std::vector<T> tmp;
    int ret = read_array_value(env, fieldObj, tmp);
    env->DeleteLocalRef(fieldObj);
    if (ret < 0) {
        return ret;
    }
    if (tmp.size() < N) {
        return HJErrInvalidParams;
    }
    for (size_t i = 0; i < N; i++) {
        value[i] = std::move(tmp[i]);
    }
    return HJ_OK;
}

template <typename T>
int deserialize_object(JNIEnv* env, jobject obj, T& value)
{
    if (!env || !obj) {
        return HJErrInvalidParams;
    }

    int i_err = HJ_OK;

    if constexpr (refl::descriptor::has_attribute<refl::attr::base_types>(refl::reflect<T>())) {
        auto bases = refl::descriptor::get_declared_base_types(refl::reflect<T>());
        auto base_descs = refl::util::reflect_types(bases);
        refl::util::for_each(base_descs, [&](auto baseDesc) {
            if (i_err < 0) {
                return;
            }
            using Base = typename decltype(baseDesc)::type;
            i_err = deserialize_object(env, obj, static_cast<Base&>(value));
        });
        if (i_err < 0) {
            return i_err;
        }
    }

    auto members = refl::descriptor::get_declared_members(refl::reflect<T>());
    refl::util::for_each(members, [&](auto member) {
        if (i_err < 0) {
            return;
        }
        if constexpr (!refl::descriptor::is_field(member)) {
            return;
        }
        auto& field = member(value);
        const std::string key = member.name.str();
        using FieldRaw = std::remove_reference_t<decltype(field)>;
        int ret = HJErrNotSupport;
        if constexpr (std::is_array_v<FieldRaw>) {
            ret = read_value(env, obj, key, field);
        } else {
            using FieldT = decay_t<decltype(field)>;
            ret = read_value<FieldT>(env, obj, key, field);
        }
        if (ret == HJErrNotExist) {
            return;
        }
        if (ret < 0) {
            i_err = ret;
        }
    });
    return i_err;
}

} // namespace refljni_detail

class HJReflCppJNIField
{
public:
    template <typename T>
    static int deserial(JNIEnv* env, jobject obj, T& value)
    {
        if constexpr (refl::trait::is_reflectable_v<T>) {
            return refljni_detail::deserialize_object(env, obj, value);
        } else {
            return HJErrNotSupport;
        }
    }
};

#else // ANDROID_LIB

class HJReflCppJNIField
{
public:
    template <typename T>
    static int deserial(void*, void*, T&)
    {
        return HJErrNotSupport;
    }
};

#endif // ANDROID_LIB

NS_HJ_END
