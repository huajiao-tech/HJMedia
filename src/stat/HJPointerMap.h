#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

template<class KEY, class POINTER_VALUE>
class HJPointerMap : public std::unordered_map<KEY, POINTER_VALUE>
{
public:
	POINTER_VALUE find2(KEY key)
	{
		auto it = this->find(key);
		if (it != this->end()) {
			return it->second;
		}
		return nullptr;
	}

	POINTER_VALUE erase2(KEY key)
	{
		auto value = find2(key);
		if (value) {
			erase(key);
		}
		return value;
	}

	POINTER_VALUE erase_begin()
	{
		auto it = this->begin();
		if (it != this->end()) {
			auto value = it->second;
			erase(it);
			return value;
		}
		return nullptr;
	}
};

NS_HJ_END
