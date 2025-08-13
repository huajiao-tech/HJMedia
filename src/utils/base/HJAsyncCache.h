#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

template <class T>
class HJAsyncCache
{
public:
	T acquire()
	{
		T data = nullptr;
		do 
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_deque.empty())
			{
				break;
			}
			else
			{
				data = m_deque.front();
				m_deque.pop_front();
				return data;
			}
		} while (false);
		return data;
		
	}
	bool recovery(T i_data)
	{
		bool bRecory = false;
		do 
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_deque.empty())
			{
				bRecory = true;
				m_deque.push_back(i_data);
				break;
			}
		} while (false);
		return bRecory;
		
	}
	void enqueue(T i_data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_deque.clear();
		m_deque.push_back(i_data);
	}
private:
	std::deque<T> m_deque;
	mutable std::mutex m_mutex;
};


NS_HJ_END
