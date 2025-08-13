#pragma once

#include "HJPrerequisites.h"
#include <variant>
#include "HJBaseUtils.h"

#ifdef ANDROID_LIB
#include <jni.h>
#endif

#if defined(HarmonyOS)
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <napi/native_api.h>
#endif

NS_HJ_BEGIN

#ifdef ANDROID_LIB
	//not good, retObjValue has type, for example is std::string; retObjValue = std::get<double>(var); is compile error
	//#define JNI_OBJ_FIELD_MAP(retObjValue) {\
	//	SL::HJJNIField::SLFieldVariant var = SL::HJJNIField::GetFieldValue(env, javaThiz, retObjValue, SL::SLUtils::getEndpointSuffix(#retObjValue)); \
	//		if (std::holds_alternative<int>(var)) {\
	//			retObjValue = std::get<int>(var);\
	//		}\
	//		else if (std::holds_alternative<int16_t>(var)){\
	//			retObjValue = std::get<int16_t>(var);\
	//		}\
	//		else if (std::holds_alternative<int64_t>(var)){\
	//			retObjValue = std::get<int64_t>(var);\
	//		}\
	//		else if (std::holds_alternative<bool>(var)){\
	//			retObjValue = std::get<bool>(var);\
	//		}\
	//		else if (std::holds_alternative<float>(var)){\
	//			retObjValue = std::get<float>(var);\
	//		}\
	//		else if (std::holds_alternative<double>(var)){\
	//			retObjValue = std::get<double>(var);\
	//		}\
	//	}\

#define JNI_OBJ_FIELD_MAP(retObjValue) \
		SL::HJJNIField::GetFieldGeneric(env, javaThiz, retObjValue, SL::SLUtils::getEndpointSuffix(#retObjValue)); \

#endif


#if defined(HarmonyOS)
	#define JNI_OBJ_FIELD_MAP(retObjValue) HJJNIField::GetFieldGeneric(env, thiz, retObjValue, HJBaseUtils::getEndpointSuffix(#retObjValue));
#endif


class HJJNIField
{
public:
	using SLFieldVariant = std::variant<int, int16_t, int64_t, bool, float, double>;
	static SLFieldVariant GetFieldValueTest(SLFieldVariant i_value, const std::string& name);
	static int m_exceptionVal;

#if defined(HarmonyOS)
	template <typename T>
	static void GetFieldGeneric(napi_env env, napi_value thiz, T& ret, const std::string& name)
	{
		if constexpr (std::is_same<T, int>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			int ret32 = 0;
			napi_get_value_int32(env, value, &ret32);
			ret = (int)ret32;
		}
		else if constexpr (std::is_same<T, int16_t>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			int32_t ret32;
			napi_get_value_int32(env, value, &ret32);
			ret = (int16_t)ret32;
		}
		else if constexpr (std::is_same<T, int64_t>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			int64_t ret64 = 0;
			bool blossless = true;
			napi_get_value_bigint_int64(env, value, &ret64, &blossless);
			ret = (int64_t)ret64;
		}
		else if constexpr (std::is_same<T, float>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			double dvalue = 0.0;
			napi_get_value_double(env, value, &dvalue);
			ret = (float)dvalue;
		}
		else if constexpr (std::is_same<T, double>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			double dvalue = 0.0;
			napi_get_value_double(env, value, &dvalue);
			ret = (double)dvalue;
		}
		else if constexpr (std::is_same<T, bool>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			bool retBool = false;
			napi_get_value_bool(env, value, &retBool);
			ret = retBool;
		}
		else if constexpr (std::is_same<T, std::string>::value)
		{
			napi_value value;
			napi_get_named_property(env, thiz, name.c_str(), &value);

			size_t strLen;
			napi_get_value_string_utf8(env, value, nullptr, 0, &strLen);

			std::vector<char> buffer(strLen + 1);

			napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), nullptr);
			ret = std::string(buffer.data());
		}
	}
#endif


#ifdef ANDROID_LIB
	
	template <typename T>
	static void GetFieldGeneric(JNIEnv* env, jobject thiz, T& ret, const std::string& name)
	{
		if constexpr (std::is_same_v<T, int>)
		{
			ret = (int)GetIntField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, int16_t>)
		{
			ret = (int16_t)GetShortField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, int64_t>)
		{
			ret = (int64_t)GetLongField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			ret = (float)GetFloatField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			ret = (double)GetDoubleField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			ret = (bool)GetBooleanField(env, thiz, name.c_str());
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			std::string rStr = GetStringField(env, thiz, name.c_str()); 
			if (!rStr.empty())
			{
				ret = rStr;
			}
		}
	}

	static SLFieldVariant GetFieldValue(JNIEnv* env, jobject thiz, SLFieldVariant i_value, const std::string& name);

	static const char* JStringToString(JNIEnv* env, jstring i_info);
	static void JStringDestroy(JNIEnv* env, const char* i_pstr, jstring i_info);

	static void SetBooleanField(JNIEnv* env, jobject thiz, uint8_t value, const std::string& name);
	static void SetByteField(JNIEnv* env, jobject thiz, int8_t value, const std::string& name);
	static void SetCharField(JNIEnv* env, jobject thiz, uint16_t value, const std::string& name);
	static void SetShortField(JNIEnv* env, jobject thiz, int16_t value, const std::string& name);
	static void SetIntField(JNIEnv* env, jobject thiz, int32_t value, const std::string& name);
	static void SetLongField(JNIEnv* env, jobject thiz, int64_t value, const std::string& name);
	static void SetFloatField(JNIEnv* env, jobject thiz, float value, const std::string& name);
	static void SetDoubleField(JNIEnv* env, jobject thiz, double value, const std::string& name);

	static int     GetIntField(JNIEnv* env, jobject thiz, const std::string& name);
	static double  GetDoubleField(JNIEnv* env, jobject thiz, const std::string& name);
	static float   GetFloatField(JNIEnv* env, jobject thiz, const std::string& name);
	static int64_t GetLongField(JNIEnv* env, jobject thiz, const std::string& name);
	static int8_t  GetByteField(JNIEnv* env, jobject thiz, const std::string& name);
	static uint16_t GetCharField(JNIEnv* env, jobject thiz, const std::string& name);
	static int16_t  GetShortField(JNIEnv* env, jobject thiz, const std::string& name);
	static uint8_t  GetBooleanField(JNIEnv* env, jobject thiz, const std::string& name);
	static jobject  GetByteBufferField(JNIEnv* env, jobject thiz, const std::string& name);
	static std::string GetStringField(JNIEnv* env, jobject thiz, const std::string& name);

	static void SetIntArrayElements(JNIEnv* env, jobject thiz, const int* values, const std::string& name);
	static void SetByteArrayElements(JNIEnv* env, jobject thiz, const int8_t* values, const std::string& name);

#endif
};


 NS_HJ_END


