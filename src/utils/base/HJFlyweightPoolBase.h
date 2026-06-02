#pragma once


#include "HJPrerequisites.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

template<typename T>
class HJFlyweightPoolBase : public HJBaseSharedObject
{
public:
    using ItemPtr = std::shared_ptr<T>;
	using ItemPtrDeque = std::deque<ItemPtr>;
	using ItemPtrDequePtr = std::shared_ptr<ItemPtrDeque>;

	using HJFlyWeightGetKeyFunc = std::function<std::string()>;
	using HJFlyWeightCreateFunc = std::function<ItemPtr()>;

	HJ_DEFINE_CREATE(HJFlyweightPoolBase);
    HJFlyweightPoolBase() = default;
    virtual ~HJFlyweightPoolBase()
    {
        m_map.clear();
    }

    ItemPtr acquire(HJFlyWeightGetKeyFunc i_getKeyFunc, HJFlyWeightCreateFunc i_createFunc)
	{
        ItemPtr val = nullptr;
        do
        {
            if (i_getKeyFunc == nullptr || i_createFunc == nullptr)
            {
                return nullptr;
            }

            std::string key = i_getKeyFunc();
            auto iter = m_map.find(key);
            if (iter == m_map.end())
            {
                m_map[key] = std::make_shared<ItemPtrDeque>();
            }

            if (m_map[key]->empty())
            {
                m_createCnt++;
                ItemPtr ptr = i_createFunc();
                if (ptr)
                {                
                    val = std::move(ptr);
                }            
            }
            else
            {
                val = m_map[key]->front();
                m_map[key]->pop_front();
            }
        } while (false);
        return val;
	}
	void recovery(ItemPtr i_ptr, HJFlyWeightGetKeyFunc i_getKeyFunc)
	{
        if (i_ptr)
        {
			if (i_getKeyFunc == nullptr)
			{
				return;
			}
			std::string key = i_getKeyFunc();

            auto iter = m_map.find(key);
            if (iter == m_map.end())
            {
                m_map[key] = std::make_shared<ItemPtrDeque>();
            }
            m_map[key]->push_back(std::move(i_ptr));
        }
        
	}
	void remove(HJFlyWeightGetKeyFunc i_getKeyFunc)
	{       
		if (i_getKeyFunc == nullptr)
		{
			return;
		}
		std::string key = i_getKeyFunc();
        auto iter = m_map.find(key);
        if (iter != m_map.end())
        {
            m_map.erase(key);
        }
	}
    void clear()
    {
        m_map.clear();
    }
protected:
    int m_createCnt = 0;
private:
	std::unordered_map<std::string, ItemPtrDequePtr> m_map;
    
};

NS_HJ_END