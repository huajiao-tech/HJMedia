#include "HJBaseUtils.h"
#include <random>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

#if (defined WIN32_LIB)
	#include "windows.h"
#endif

NS_HJ_BEGIN

bool HJBaseUtils::isHttpUrl(const std::string& i_url)
{
	bool bHttp = false;
	do 
	{
		if (isMatchTag(i_url, "http://") || isMatchTag(i_url, "https://"))
		{
			bHttp = true;
			break;
		}
	} while (false);
	return bHttp;
}
bool HJBaseUtils::isMatchTag(const std::string& i_url, const std::string& i_tag)
{
	size_t tagSize = i_tag.size();
	if ((i_url.size() >= tagSize) && (i_url.compare(0, tagSize, i_tag) == 0)) 
		return true;
	return false;
}

std::string HJBaseUtils::Utf8ToGbk(const char* src_str)
{
#if (defined WIN32_LIB)
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK)
		delete[] wszGBK;
	if (szGBK)
		delete[] szGBK;
	return strTemp;
#else
	return src_str;
#endif
}
std::string HJBaseUtils::GbkToUtf8(const char* src_str)
{
#if (defined WIN32_LIB)
	int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	std::string strTemp = str;
	if (wstr)
		delete[] wstr;
	if (str)
		delete[] str;
	return strTemp;
#else
	return src_str;
#endif
}

void HJBaseUtils::getFormatTimeFromMils(int i_nCurTime, int* o_hour, int* o_mins, int* o_sec)
{
	int hour = i_nCurTime / (3600 * 1000);
	int mins = (i_nCurTime - hour * 3600 * 1000) / (60 * 1000);
	int sec = (i_nCurTime - hour * 3600 * 1000 - mins * 60 * 1000) / 1000;

	*o_hour = hour;
	*o_mins = mins;
	*o_sec = sec;
}
std::string HJBaseUtils::getFormatTimeFromMils(int i_nTime)
{
	int hour = 0;
	int mins = 0;
	int sec = 0;
	getFormatTimeFromMils(i_nTime, &hour, &mins, &sec);

	std::string timestr = std::to_string(hour) + ":" + std::to_string(mins) + ":" + std::to_string(sec);
	return timestr;
}
std::string HJBaseUtils::getRealPath(const std::string& i_path)
{
	if (i_path.empty())
	{
		return i_path;
	}
	if (i_path.back() == '/' || i_path.back() == '\\')
	{
		return i_path;
	}
	return i_path + "/";
}

bool HJBaseUtils::m_bRandomInit = false;
void HJBaseUtils::randomInit()
{
	if (!m_bRandomInit)
	{
		std::srand(std::time(nullptr));
		m_bRandomInit = true;
	}
}
int HJBaseUtils::getRandomValue(int min, int max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(min, max);
	int randomValue = dis(gen);
	return randomValue;
}
int HJBaseUtils::getRandom(int min, int max)
{
	return min + std::rand() % (max - min);
}
std::string HJBaseUtils::getFunctionName(const std::string& i_func)
{
#ifndef _WIN32
	return std::string(i_func.c_str());
#else
	auto pos = strrchr(i_func.c_str(), ':');
	return std::string(pos ? pos + 1 : i_func.c_str());
#endif
}
std::string HJBaseUtils::getFunName(const char *i_func)
{
	if (!i_func)
	{
		return "";
	}
	return getFunctionName(std::string(i_func));
}

std::string HJBaseUtils::getHostFromUrl(const std::string& i_url)
{
	std::string::size_type start = i_url.find("://");
	if (start == std::string::npos) {
		return ""; 
	}
	start += 3; 

	std::string::size_type end = i_url.find('/', start);
	if (end == std::string::npos) {
		return i_url.substr(start); 
	}

	return i_url.substr(start, end - start);
}
std::string HJBaseUtils::getEndpointSuffix(const std::string& i_url)
{
	std::string value = "";
	size_t pos = i_url.find('.');  
	if ((pos != std::string::npos) && ((pos + 1) < i_url.size()))
	{
		value = i_url.substr(pos + 1);
	}
	return value;
}
std::string HJBaseUtils::getPathFromUrl(const std::string& i_url)
{
	std::string::size_type start = i_url.find("://");
	if (start == std::string::npos) {
		return ""; 
	}
	start += 3; 

	start = i_url.find('/', start);
	if (start == std::string::npos) {
		return ""; 
	}

	return i_url.substr(start);
}

//bool HJBaseUtils::isHttpUrl(const std::string& i_url)
//{
//	const std::string https_prefix = "https://";
//	const std::string http_prefix = "http://";
//	return ((i_url.compare(0, https_prefix.length(), https_prefix) == 0)
//		|| ((i_url.compare(0, http_prefix.length(), http_prefix) == 0)));
//}
bool HJBaseUtils::isSuffixFlv(const std::string& i_url)
{
	const std::string flv_suffix = ".flv";
	return (i_url.size() >= flv_suffix.length() &&
		i_url.compare(i_url.size() - flv_suffix.length(), flv_suffix.length(), flv_suffix) == 0);
}
std::string HJBaseUtils::formatU8ArrayToStr(const std::string& i_name, unsigned char* i_pBuffer, int i_nSize)
{
	size_t size = (size_t)i_nSize;
	std::ostringstream oss;
	oss << "uint8_t test[" << size << "] = {";
	for (size_t i = 0; i < size; ++i) {
		oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i_pBuffer[i]);
		if (i < size - 1) {
			oss << ", ";
		}
	}
	oss << "};";
	std::string formattedString = oss.str();
	return formattedString;
}

std::vector<char> HJBaseUtils::readBinaryFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) 
	{
		return {};
	}
	file.seekg(0, std::ios::end);
	if (file.fail()) 
	{
		return {};
	}
	size_t size = file.tellg();
	file.seekg(0);
	if (file.fail()) 
	{
		return {};
	}
	std::vector<char> buffer(size);
	file.read(buffer.data(), size);
	return buffer;
}
std::string HJBaseUtils::readFileToString(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return "";
	}
	file.seekg(0, std::ios::end);
	if (file.fail())
	{
		return {};
	}
	size_t size = file.tellg();
	file.seekg(0);
	if (file.fail())
	{
		return {};
	}
	std::string buffer(size, '\0');
	file.read(&buffer[0], size);
	return buffer;
}
std::vector<std::string> HJBaseUtils::getSortedFiles(const std::string& i_dirPath, const std::string& i_prefix, bool i_bUseAbsPath)
{
	std::vector<std::string> files;

	std::string path = getRealPath(i_dirPath);
#if !defined(__APPLE__) //to do ??
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		std::string filename = entry.path().filename().string();
		if (filename.find(i_prefix) == 0) 
		{
			std::string target = filename;
			if (i_bUseAbsPath)
			{
				target = path + filename;
			}
			files.push_back(target);
		}
	}
#endif
	std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b)
		{
		int num_a = std::stoi(a.substr(a.rfind('_') + 1));
		int num_b = std::stoi(b.substr(b.rfind('_') + 1));
		return num_a < num_b;
		});
	return files;
}
std::string HJBaseUtils::combineUrl(const std::string& i_path, const std::string& i_fileName)
{
	std::string path = getRealPath(i_path);
	if (path.empty())
	{
		return "";
	}
	if (i_fileName.empty())
	{
		return "";
	}
	return path + i_fileName;
}

NS_HJ_END
