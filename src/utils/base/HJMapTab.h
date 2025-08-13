#pragma once

#include "HJPrerequisites.h"
#include <unordered_map>
#include <string>
#include <any>

NS_HJ_BEGIN

class HJMapTab : public std::unordered_map<std::string, std::any>
{
public:
	HJ_DEFINE_CREATE(HJMapTab);

	virtual ~HJMapTab()
	{
		clear();
	}

	bool contains(const std::string& key) const
	{
		return find(key) != end();
	}

	template<typename ValueType>
	ValueType getValue(const std::string& key) const
	{
		if (contains(key))
		{
			auto it = find(key);

			const std::type_info& typeInfo = it->second.type();
			if (typeInfo == typeid(ValueType))
			{
				return std::any_cast<ValueType>(it->second);			
			}
			else
			{
				throw std::runtime_error("type is not match");
			}		
		}
		else
		{
			throw std::runtime_error("Key not found");
		}
	}

	int getValInt(const std::string& key) const
	{
		return getValue<int>(key);
	}
	std::string getValString(const std::string& key) const
	{
		return getValue<std::string>(key);
	}
	uint32_t getValUInt(const std::string& key) const
	{
		return getValue<uint32_t>(key);
	}
	float getValFloat(const std::string& key) const
	{
		return getValue<float>(key);
	}
	double getValDouble(const std::string& key) const
	{
		return getValue<double>(key);
	}
	bool getValBool(const std::string& key) const
	{
		return getValue<bool>(key);
	}
	int64_t getValInt64(const std::string& key) const
	{
		return getValue<int64_t>(key);
	}
	uint64_t getValUInt64(const std::string& key) const
	{
		return getValue<uint64_t>(key);
	}

};

NS_HJ_END