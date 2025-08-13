

#include "HJJNIField.h"

NS_HJ_BEGIN

int HJJNIField::m_exceptionVal = -65536;

HJJNIField::SLFieldVariant HJJNIField::GetFieldValueTest(HJJNIField::SLFieldVariant i_value, const std::string& name)
{
	HJJNIField::SLFieldVariant ret;
	if (std::holds_alternative<int>(i_value))
	{
		ret = 100;
	}
	else if (std::holds_alternative<int16_t>(i_value))
	{
		ret = 18;
	}
	else if (std::holds_alternative<int64_t>(i_value))
	{
		ret = 100;
	}
	else if (std::holds_alternative<bool>(i_value))
	{
		ret = true;
	}
	else if (std::holds_alternative<float>(i_value))
	{
		ret = 5.f;
	}
	else if (std::holds_alternative<double>(i_value))
	{
		ret = 8.0;
	}
	else
	{
		return (int)m_exceptionVal;
	}
	return ret;

#if 0
	auto process = [](auto&& val) {

		HJJNIField::SLFieldVariant ret;

		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int>) 
		{
			ret = 1;
			//std::cout << "处理整数: " << val * 2 << std::endl;
		}
		else if constexpr (std::is_same_v<T, float>) {
			//std::cout << "处理浮点数: " << val + 1.0 << std::endl;
			ret = 5.f;
		}
		else if constexpr (std::is_same_v<T, double>) {
			//std::cout << "处理浮点数: " << val + 1.0 << std::endl;
			ret = 1.0;
		} 
		else if constexpr (std::is_same_v<T, std::string>) {
			ret = "hello";
			//std::cout << "处理字符串: " << val.substr(0, 3) << std::endl;
		}
		return ret;
	};
	return std::visit(process, i_value);   
#endif
}

#ifdef ANDROID_LIB

HJJNIField::SLFieldVariant HJJNIField::GetFieldValue(JNIEnv* env, jobject thiz, HJJNIField::SLFieldVariant i_value, const std::string& name)
{
	HJJNIField::SLFieldVariant ret;
	if (std::holds_alternative<int>(i_value))
	{
		ret = (int)GetIntField(env, thiz, name.c_str());
	}
	else if (std::holds_alternative<int16_t>(i_value))
	{
		ret = (int16_t)GetShortField(env, thiz, name.c_str());
	}
	else if (std::holds_alternative<int64_t>(i_value))
	{
		ret = (int64_t)GetLongField(env, thiz, name.c_str());
	}
	else if (std::holds_alternative<bool>(i_value))
	{
		ret = (bool)GetBooleanField(env, thiz, name.c_str());
	}
	else if (std::holds_alternative<float>(i_value))
	{
		ret = (float)GetFloatField(env, thiz, name.c_str());
	}
	else if (std::holds_alternative<double>(i_value))
	{
		ret = (double)GetDoubleField(env, thiz, name.c_str());
	}
	else
	{
		ret = (int)m_excepationVal;
	}
	return ret;
}

const char* HJJNIField::JStringToString(JNIEnv* env, jstring i_info)
{
	jboolean   isCopy;
	const char* strParams = NULL;
	strParams = env->GetStringUTFChars(i_info, &isCopy);
	return strParams;
}

void HJJNIField::JStringDestroy(JNIEnv* env, const char* i_pstr, jstring i_info)
{
	if (i_pstr)
	{
		env->ReleaseStringUTFChars(i_info, i_pstr);
	}
}

void HJJNIField::SetBooleanField(JNIEnv* env, jobject thiz, uint8_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetBooleanField(thiz, env->GetFieldID(cls, name.c_str(), "Z"), (jboolean)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetByteField(JNIEnv* env, jobject thiz, int8_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetByteField(thiz, env->GetFieldID(cls, name.c_str(), "B"), (jbyte)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetCharField(JNIEnv* env, jobject thiz, uint16_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetCharField(thiz, env->GetFieldID(cls, name.c_str(), "C"), (jchar)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetShortField(JNIEnv* env, jobject thiz, int16_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetShortField(thiz, env->GetFieldID(cls, name.c_str(), "S"), (jshort)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetIntField(JNIEnv* env, jobject thiz, int32_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetIntField(thiz, env->GetFieldID(cls, name.c_str(), "I"), (jint)value);
	env->DeleteLocalRef(cls);
}
int HJJNIField::GetIntField(JNIEnv* env, jobject thiz, const std::string& name)
{
	int value = 0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetIntField(thiz, env->GetFieldID(cls, name.c_str(), "I"));
	env->DeleteLocalRef(cls);
	return value;
}

double  HJJNIField::GetDoubleField(JNIEnv* env, jobject thiz, const std::string& name)
{
	double value = 0.0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetDoubleField(thiz, env->GetFieldID(cls, name.c_str(), "D"));
	env->DeleteLocalRef(cls);
	return value;
}
float   HJJNIField::GetFloatField(JNIEnv* env, jobject thiz, const std::string& name)
{
	float value = 0.f;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetFloatField(thiz, env->GetFieldID(cls, name.c_str(), "F"));
	env->DeleteLocalRef(cls);
	return value;
}
int64_t HJJNIField::GetLongField(JNIEnv* env, jobject thiz, const std::string& name)
{
	int64_t value = 0;
	jclass cls = env->GetObjectClass(thiz);
	jfieldID fid = env->GetFieldID(cls, name.c_str(), "J");
	value = env->GetLongField(thiz, fid);
	env->DeleteLocalRef(cls);
	return value;
}
int8_t  HJJNIField::GetByteField(JNIEnv* env, jobject thiz, const std::string& name)
{
	int8_t value = 0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetByteField(thiz, env->GetFieldID(cls, name.c_str(), "B"));
	env->DeleteLocalRef(cls);
	return value;
}
uint16_t HJJNIField::GetCharField(JNIEnv* env, jobject thiz, const std::string& name)
{
	uint16_t value = 0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetCharField(thiz, env->GetFieldID(cls, name.c_str(), "C"));
	env->DeleteLocalRef(cls);
	return value;
}
int16_t  HJJNIField::GetShortField(JNIEnv* env, jobject thiz, const std::string& name)
{
	int16_t value = 0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetShortField(thiz, env->GetFieldID(cls, name.c_str(), "S"));
	env->DeleteLocalRef(cls);
	return value;
}
uint8_t  HJJNIField::GetBooleanField(JNIEnv* env, jobject thiz, const std::string& name)
{
	uint8_t value = 0;
	jclass cls = env->GetObjectClass(thiz);
	value = env->GetBooleanField(thiz, env->GetFieldID(cls, name.c_str(), "Z"));
	env->DeleteLocalRef(cls);
	return value;
}
std::string HJJNIField::GetStringField(JNIEnv* env, jobject thiz, const std::string& name)
{
	std::string ret = "";
	do
	{
		jclass cls = env->GetObjectClass(thiz);
		if (cls == NULL) {
			break;
		}

		const char* fieldSig = "Ljava/lang/String;";
		jfieldID fieldId = env->GetFieldID(cls, name.c_str(), fieldSig);
		if (fieldId == NULL) {
			env->DeleteLocalRef(cls);
			break;
		}

		jstring jstrValue = (jstring)env->GetObjectField(thiz, fieldId);
		if (jstrValue != NULL) {
			const char* cStrValue = env->GetStringUTFChars(jstrValue, NULL);
			if (cStrValue != NULL) {
				ret = cStrValue;
				env->ReleaseStringUTFChars(jstrValue, cStrValue);
			}
			env->DeleteLocalRef(jstrValue);
		}
		env->DeleteLocalRef(cls);
	} while (false);
	return ret;
}
jobject  HJJNIField::GetByteBufferField(JNIEnv* env, jobject thiz, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	jfieldID fid = env->GetFieldID(cls, name.c_str(), "Ljava/nio/ByteBuffer;");
	jobject value = env->GetObjectField(thiz, fid);
	env->DeleteLocalRef(cls);
	return value;
}
void HJJNIField::SetLongField(JNIEnv* env, jobject thiz, int64_t value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetLongField(thiz, env->GetFieldID(cls, name.c_str(), "J"), (jlong)value);
	env->DeleteLocalRef(cls);
}
void HJJNIField::SetFloatField(JNIEnv* env, jobject thiz, float value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetFloatField(thiz, env->GetFieldID(cls, name.c_str(), "F"), (jfloat)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetDoubleField(JNIEnv* env, jobject thiz, double value, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	env->SetDoubleField(thiz, env->GetFieldID(cls, name.c_str(), "D"), (jdouble)value);
	env->DeleteLocalRef(cls);
}

void HJJNIField::SetIntArrayElements(JNIEnv* env, jobject thiz, const int32_t* values, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	jfieldID jvaluesID = env->GetFieldID(cls, name.c_str(), "[I");
	auto jvalueArray = (jintArray)env->GetObjectField(thiz, jvaluesID);
	jboolean isCopy;
	jint* jvalues = env->GetIntArrayElements(jvalueArray, &isCopy);
	jint jlength = env->GetArrayLength(jvalueArray);
	for (int i = 0; i < jlength; i++) {
		jvalues[i] = values[i];
	}

	env->ReleaseIntArrayElements(jvalueArray, jvalues, 0);
	env->DeleteLocalRef(jvalueArray);
	env->DeleteLocalRef(cls);
}

void SLJNIField::SetByteArrayElements(JNIEnv* env, jobject thiz, const int8_t* values, const std::string& name)
{
	jclass cls = env->GetObjectClass(thiz);
	jfieldID jvaluesID = env->GetFieldID(cls, name.c_str(), "[B");
	auto jvalueArray = (jbyteArray)env->GetObjectField(thiz, jvaluesID);
	jboolean isCopy;
	jbyte* jvalues = env->GetByteArrayElements(jvalueArray, &isCopy);
	jint jlength = env->GetArrayLength(jvalueArray);
	for (int i = 0; i < jlength; i++) {
		jvalues[i] = values[i];
	}

	env->ReleaseByteArrayElements(jvalueArray, jvalues, 0);
	env->DeleteLocalRef(jvalueArray);
	env->DeleteLocalRef(cls);
}

#endif

NS_HJ_END

