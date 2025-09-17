#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN
  
class HJBaseUtils
{
public:
	static std::string Utf8ToGbk(const char* src_str);
	static std::string GbkToUtf8(const char* src_str);

	static void getFormatTimeFromMils(int i_nCurTime, int* o_hour, int* o_mins, int* o_sec);
	static std::string getFormatTimeFromMils(int i_nTime);

	static std::string getRealPath(const std::string &i_path);
    
	static void randomInit();
	//[min, max)
	static int  getRandom(int min, int max);
	static bool m_bRandomInit;

	static std::string getFunctionName(const std::string& i_func);
	static std::string getFunName(const char* i_func);

	static std::string getHostFromUrl(const std::string& i_url);
	static std::string getPathFromUrl(const std::string& i_url);

	static std::string getEndpointSuffix(const std::string& i_url);

	//static bool isHttpUrl(const std::string& i_url);
	static bool isSuffixFlv(const std::string& i_url);

	static int getRandomValue(int min, int max);

	static bool isHttpUrl(const std::string& i_url);
	static bool isMatchTag(const std::string& i_url, const std::string& i_tag);

	static std::string formatU8ArrayToStr(const std::string& i_name, unsigned char* i_pBuffer, int i_nSize);

	static std::vector<char> readBinaryFile(const std::string& path);
    static std::string readFileToString(const std::string &path);
	static std::vector<std::string> getSortedFiles(const std::string& i_dirPath, const std::string& i_prefix, bool i_bUseAbsPath = true);
	static std::string combineUrl(const std::string& i_path, const std::string& i_fileName);

	static std::string getPlatform();

	template<typename T>
	static std::deque<T> copyDeque(const std::deque<T>& i_deque)
	{
		std::deque<T> ret;
		for (auto& item : i_deque)
		{
			ret.push_back(item);
		}
		return ret;
	}
};

NS_HJ_END

