//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJDefines.h"
#include "HJSTDHeaders.h"

//***********************************************************************************//
#define NS_HJ_BEGIN        namespace HJ{
#define NS_HJ_END          }
#define NS_HJ_USING        using namespace HJ;

#define HJ_ISTYPE(var, type)       (typeid(type) == typeid(var))
#define HJANY_ISTYPE(var, decl)    (typeid(decl) == var.type())
#define HJ_TYPE_NAME(type)         typeid(type).name()
#define HJ_VAR_NAME(variable)      (#variable)

#define HJ_AUTO_LOCK(mtx)    std::lock_guard<decltype(mtx)> lock(mtx);
#define HJ_AUTOU_LOCK(mtx)   std::unique_lock<decltype(mtx)> lock(mtx);
#define HJ_AUTOS_LOCK(mtx)   std::shared_lock<decltype(mtx)> lock(mtx);

#define HJ_INSTANCE_IMP(class_name, ...) \
class_name& class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}

#define HJ_INSTANCE_DECL(ClassName) \
    static std::shared_ptr<ClassName>& getInstance(); \
    static void destoryInstance();

#define HJ_INSTANCE(ClassName, ...) \
static std::shared_ptr<ClassName> s_instance = nullptr; \
std::shared_ptr<ClassName>& ClassName::getInstance() { \
    if(!s_instance) { \
        s_instance = std::make_shared<ClassName>(__VA_ARGS__); \
    } \
    return s_instance; \
} \
void ClassName::destoryInstance() { \
    s_instance = nullptr; \
}

#define HJDeclareVariableFunction(type, variable, defaultValue) \
    type m_##variable = defaultValue; \
    void set##variable(const type value) { \
        m_##variable = value;\
    } \
    type& get##variable() { \
        return m_##variable; \
    }

#define HJDeclareVariable(type, variable, defaultValue) \
    type m_##variable{ defaultValue };

#define HJDeclareFunction(type, variable) \
    void variable(const type value) { \
        m_##variable = value;\
    } \
    const type variable() const { \
        return m_##variable; \
    }


#define HJ_DECLARE_PUWTR(class) \
    using Ptr = std::shared_ptr<class>; \
    using Utr = std::unique_ptr<class>; \
    using Wtr = std::weak_ptr<class>;

#ifdef _WIN32
#define HJ_INLINE
#else
#define HJ_INLINE __attribute__((__always_inline__)) inline
#endif

#if !defined(HJ_OS_LINUX) && !defined(HJ_OS_DARWIN)
    #ifndef bzero
    #   define bzero(ptr,size)  memset((void*)(ptr), 0, (size_t)(size));
    #endif //bzero
#endif

#if defined(_WIN32)
//
#if !defined(strcasecmp)
    #define strcasecmp _stricmp
#endif
//
#ifndef ssize_t
    #ifdef _WIN64
        #define ssize_t int64_t
    #else
        #define ssize_t int32_t
    #endif
#endif
//
#ifndef PATH_MAX
#   define PATH_MAX 1024
#endif // !PATH_MAX

#endif //_WIN32

#define HJ_ALIGN(x, a)     (((x)+(a)-1)&~((a)-1))
#define HJ_ALIGN4(x)       HJ_ALIGN(x,4)
#define HJ_ALIGN8(x)       HJ_ALIGN(x,8)
#define HJ_ALIGN16(x)      HJ_ALIGN(x,16)
#define HJ_ALIGN32(x)      HJ_ALIGN(x,32)

#ifdef _WIN32
#   define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#   define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif

#define HJValidPtr(p)      (p && (*p))


#define HJ_BE_32(x) (((uint32_t)(((uint8_t*)(x))[0]) << 24) |  \
                             (((uint8_t*)(x))[1]  << 16) |  \
                             (((uint8_t*)(x))[2]  <<  8) |  \
                              ((uint8_t*)(x))[3])

#define HJ_BE_64(x) (((uint64_t)(((uint8_t*)(x))[0]) << 56) |  \
                  ((uint64_t)(((uint8_t*)(x))[1]) << 48) |  \
                  ((uint64_t)(((uint8_t*)(x))[2]) << 40) |  \
                  ((uint64_t)(((uint8_t*)(x))[3]) << 32) |  \
                  ((uint64_t)(((uint8_t*)(x))[4]) << 24) |  \
                  ((uint64_t)(((uint8_t*)(x))[5]) << 16) |  \
                  ((uint64_t)(((uint8_t*)(x))[6]) <<  8) |  \
                  ((uint64_t)( (uint8_t*)(x))[7]))

#define HJ_WB32(p, val)    {                    \
    ((uint8_t*)(p))[0] = ((val) >> 24) & 0xff;  \
    ((uint8_t*)(p))[1] = ((val) >> 16) & 0xff;  \
    ((uint8_t*)(p))[2] = ((val) >> 8) & 0xff;   \
    ((uint8_t*)(p))[3] = (val) & 0xff;          \
    }

#define HJ_WB64(p, val)    {                    \
    HJ_WB32(p, (val) >> 32)                     \
    HJ_WB32(p + 4, val)                         \
    }

#define HJ_BE_FOURCC(ch0, ch1, ch2, ch3)           \
    ( (uint32_t)(unsigned char)(ch3)        |   \
     ((uint32_t)(unsigned char)(ch2) <<  8) |   \
     ((uint32_t)(unsigned char)(ch1) << 16) |   \
     ((uint32_t)(unsigned char)(ch0) << 24) )


#define HJEnumToStringFuncDecl(name) const std::string name##ToString(name val);

#define HJEnumToStringItem(item) {item, HJ_VAR_NAME(item)}

#define HJEnumToStringFuncImpl(name) \
const std::string name##ToString(name val) \
{ \
	auto it = g_##name##String.find(val); \
	if (it != g_##name##String.end()) { \
		return it->second; \
	} \
	return ""; \
}

#define HJEnumToStringFuncImplBegin(name) static const std::unordered_map<name, std::string> g_##name##String = {

#define HJEnumToStringFuncImplEnd(name) \
    }; \
    HJEnumToStringFuncImpl(name); 

//***********************************************************************************//
#if defined(_MSC_VER)
#    define HJ_INT16_MIN       (-32767i16 - 1)
#    define HJ_INT16_MAX       32767i16
#    define HJ_INT_MIN         (-2147483647i32 - 1)            /* minimum (signed) int value */
#    define HJ_INT_MAX         2147483647i32                    /* maximum (signed) int value */
#    define HJ_UINT_MAX        0xffffffffui32                    /* maximum unsigned int value */
#    define HJ_INT64_MAX       9223372036854775807i64            /* maximum signed long long int value */
#    define HJ_INT64_MIN       (-9223372036854775807i64 - 1)    /* minimum signed long long int value */
#    define HJ_UINT64_MAX      0xffffffffffffffffui64            /* maximum unsigned long long int value */
#else
#    define HJ_INT16_MIN       (-32767 - 1)
#    define HJ_INT16_MAX       32767
#    define HJ_INT_MIN         (-2147483647 - 1)
#    define HJ_INT_MAX         2147483647
#    define HJ_UINT_MAX        0xffffffffU
#    define HJ_INT64_MAX       9223372036854775807LL
#    define HJ_INT64_MIN       (-9223372036854775807LL - 1)
#    define HJ_UINT64_MAX      0xffffffffffffffffULL
#endif

#define HJ_LL(x) x##LL
#define HJ_ULL(x) x##ULL
#define HJ_LL_FORMAT "ll"  // As in "%lld". Note that "q" is poor form also.
#define HJ_LL_FORMAT_W L"ll"

const uint8_t HJUInt8max{0xFF};
const uint16_t HJUInt16max{0xFFFF};
const uint32_t HJUInt32max{0xFFFFFFFF};
const uint64_t HJUInt64max{ HJ_ULL(0xFFFFFFFFFFFFFFFF) };
const int8_t HJInt8min{~0x7F};
const int8_t HJInt8max{0x7F};
const int16_t HJInt16min{~0x7FFF};
const int16_t HJInt16max{0x7FFF};
const int32_t HJInt32min{~0x7FFFFFFF};
const int32_t HJInt32max{0x7FFFFFFF};
const int64_t HJInt64min{ HJ_LL(~0x7FFFFFFFFFFFFFFF) };
const int64_t HJInt64max{ HJ_LL(0x7FFFFFFFFFFFFFFF) };

#ifndef HJ_FLT_EPSILON
#   define HJ_FLT_EPSILON         1.192092896e-07F
#endif // FLT_EPSILON
#ifndef HJ_DBL_EPSILON
#   define HJ_DBL_EPSILON         2.2204460492503131e-016
#endif

#define HJ_DBL_MAX             1.7976931348623158e+308 /* max value */
#define HJ_DBL_MIN             2.2250738585072014e-308 /* min positive value */
#define HJ_FLT_MAX             3.402823466e+38F        /* max value */
#define HJ_FLT_MIN             1.175494351e-38F        /* min positive value */

#define HJ_POSITIVE_INFINITY   (3.402823466e+38F)
#define HJ_NEGATIVE_INFINITY   (1.175494351e-38F)

#define HJ_TIME_SCALE          (int64_t)(10000000)
#define HJ_MSTIME_SCALE        (int64_t)(1000)
#define HJ_NOPTS_VALUE         (int64_t)(0x8000000000000000)
#define HJ_NOTS_VALUE          (int64_t)(0x8000000000000000)

#define HJ_NS_PER_SEC          (int64_t)(1000000000)
#define HJ_US_PER_SEC          (int64_t)(1000000)
#define HJ_MS_PER_SEC          (int64_t)(1000)
#define HJ_US_PER_MS           (int64_t)(1000)

#define HJ_SEC_TO_MS(sec)      (uint64_t)((sec)*HJ_MS_PER_SEC + 0.5)
#define HJ_MS_TO_SEC(ms)       ((ms)/(double)HJ_MS_PER_SEC)
#define HJ_SEC_TO_US(sec)      (uint64_t)((sec)*HJ_US_PER_SEC + 0.5)
#define HJ_US_TO_SEC(us)       ((us)/(double)HJ_US_PER_SEC)
#define HJ_SEC_TO_NS(sec)      (uint64_t)((sec)*HJ_NS_PER_SEC + 0.5)
#define HJ_NS_TO_SEC(us)       ((ns)/(double)HJ_NS_PER_SEC)
#define HJ_MS_TO_US(ms)        (uint64_t)((ms)*HJ_US_PER_MS + 0.5)
#define HJ_US_TO_MS(us)        ((us)/(double)HJ_US_PER_MS)

#define HJ_ANIM_INTERVAL       (1.0f/30.0f)
#define HJ_ANIMF15_INTERVAL    (1.0f/15.0f)
#define HJ_ANIMF60_INTERVAL    (1.0f/60.0f)
#define HJ_INTERVAL_MS         (1000.0f/30.0f)

#define HJ_INTERVAL_STEPS_MIN  1
#define HJ_INTERVAL_STEPS_MAX  100

#define HJ_BPS_TO_KBPS(val)    ((val)/(double)1024)
#define HJ_BPS_TO_MBPS(val)    ((val)/(double)(1024 * 1024))
#define HJ_KBPS_TO_BPS(val)    (uint64_t)((val)*1024 + 0.5)
#define HJ_MBPS_TO_BPS(val)    (uint64_t)((val)*1024*1024 + 0.5)

#define HJ_ARRAY_ELEMS(a)      (sizeof(a) / sizeof((a)[0]))

#define HJ_MAKE_ID_STR(ID)                     {ID,#ID}
#define HJ_MAKE_ID_STR_EX(ID, sz_ID)           {ID, sz_ID}
#define HJ_MAKE_ID_STR_EX2(ID, sz_ID, sz_EX)   {ID, sz_ID, sz_EX}

#define HJ_ABS(a)              ((a) >= 0 ? (a) : (-(a)))
#define HJ_SIGN(a)             ((a) > 0 ? 1 : -1)

#define HJ_MAX(a,b)            ((a) > (b) ? (a) : (b))
#define HJ_MAX3(a,b,c)         HJ_MAX(HJ_MAX(a,b),c)
#define HJ_MIN(a,b)            ((a) > (b) ? (b) : (a))
#define HJ_MIN3(a,b,c)         HJ_MIN(HJ_MIN(a,b),c)

#define HJ_CLIP(xvalue, xmin, xmax)            (((xvalue) < (xmin)) ? (xmin) : (((xvalue) > (xmax)) ? (xmax) : (xvalue) ))

#define HJ_BEYOND(xvalue, xmin, xmax)          (((xvalue < xmin) || (xvalue > xmax)) ? true : false)
#define HJ_BEYONDLR(xvalue, xmin, xmax)        (((xvalue <= xmin) || (xvalue >= xmax)) ? true : false)
#define HJ_LIMITED(xvalue, xmin, xmax)         (((xvalue >= xmin) && (xvalue <= xmax)) ? true : false)
#define HJ_LIMITEDLR(xvalue, xmin, xmax)       (((xvalue > xmin) && (xvalue < xmax)) ? true : false)
#define HJ_LIMITEDL(xvalue, xmin, xmax)        (((xvalue > xmin) && (xvalue <= xmax)) ? true : false)
#define HJ_LIMITEDR(xvalue, xmin, xmax)        (((xvalue >= xmin) && (xvalue < xmax)) ? true : false)

#define HJ_PRECISION           0.00001f
#define HJ_IS_PRECISION(xval, precis)          ((((xval) >= -(precis)) && ((xval) <= (precis))) ? true : false)
#define HJ_IS_PRECISION_DEFAULT(xval)          HJ_IS_PRECISION(xval, HJ_PRECISION)
#define HJ_IS_EQUAL(v0, v1)                    HJ_IS_PRECISION(v0 - v1, HJ_PRECISION)

#define HJ_NOLINE_LINE(val)                    ((val < 0.f) ? (1.f / val) : val)
#define HJ_OneReciprocal(val)                  ((val < 1.f) ? ((-1.f / val) + 1.f) : (val - 1.f))
#define HJ_OneReciprocalInvert(val)            ((val < 0.f) ? (-1.f / (val - 1.0f)) : (val + 1.f))

#define HJ_INVALID_HAND    0x00000000          //size_t

//
#ifndef HJMEDIAKIT_HAS_RTTI
// Detect if RTTI is disabled in the compiler.
#if defined(__clang__) && defined(__has_feature)
#       define HJMEDIAKIT_HAS_RTTI __has_feature(cxx_rtti)
#elif defined(__GNUC__) && !defined(__GXX_RTTI)
#       define HJMEDIAKIT_HAS_RTTI 0
#elif defined(_MSC_VER) && !defined(_CPPRTTI)
#       define HJMEDIAKIT_HAS_RTTI 0
#else
#       define HJMEDIAKIT_HAS_RTTI 1
#endif
#endif  // HJMEDIAKIT_HAS_RTTI

//
//#ifndef HJEDIAKIT_EXPORT
#if defined(HJ_STATIC)
#      define HJ_EXPORT_API
#else
#if (defined( __WIN32__ ) || defined( _WIN32 ))
#   ifdef HJEDIAKIT_EXPORT
#      define HJ_EXPORT_API __declspec(dllexport)
#   else
#      define HJ_EXPORT_API __declspec(dllimport)
#   endif

#   ifndef HJEDIAKIT_NO_EXPORT
#       define HJEDIAKIT_NO_EXPORT
#   endif
#else //IOS Android
#   ifdef HJEDIAKIT_EXPORT
#       define HJ_EXPORT_API __attribute__((visibility("default")))
#   else
#       define HJ_EXPORT_API
#   endif

#  ifndef HJEDIAKIT_NO_EXPORT
#       define HJEDIAKIT_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif //#if (defined( __WIN32__ ) || defined( _WIN32 ))
#endif
//#endif //HJEDIAKIT_EXPORT
