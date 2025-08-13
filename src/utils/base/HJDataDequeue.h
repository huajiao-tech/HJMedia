#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

//header file, not in cpp file;
template <class T>
class HJDataDeque
{
public:
	void push_back(T* i_packet)
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.push_back(i_packet);
	}
	int size() const
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		return (int)m_deque.size();
	}
	T* pop()
	{
		T* packet = NULL;
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		typename std::deque<T*>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
			m_deque.erase(it);
		}
		return packet;
	}
	void clear()
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		for (typename std::deque<T*>::iterator it = m_deque.begin(); it != m_deque.end(); it++)
		{
			T* packet = *it;
			delete packet;
		}
		m_deque.clear();
	}
	T* getFirst()
	{
		T* packet = NULL;
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		typename std::deque<T*>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
		}
		return packet;
	}
	void eraseFirst()
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.pop_front();
	}
private:
	std::deque<T*> m_deque;
	mutable std::recursive_mutex m_mutex;
};

template <class T>
class HJDataDequeNoLock
{
public:
	void push_back(T i_packet)
	{
		m_deque.push_back(i_packet);
	}
	void push_back_move(T i_packet)
	{
		m_deque.push_back(std::move(i_packet));
	}
	int size() const
	{
		return (int)m_deque.size();
	}
	bool isEmpty() const
	{
		return m_deque.empty();
	}
	T pop()
	{
		T packet = nullptr;
		typename std::deque<T>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
			m_deque.erase(it);
		}
		return packet;
	}
	void clear()
	{
		m_deque.clear();
	}
	T getFirst()
	{
		T packet = nullptr;
		typename std::deque<T>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
		}
		return packet;
	}
	void eraseFirst()
	{
		m_deque.pop_front();
	}
private:
	std::deque<T> m_deque;
};


template <class T>
class HJSpDataDeque
{
public:
	void push_back(T i_packet)
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.push_back(i_packet);
	}
	void push_back_move(T i_packet)
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.push_back(std::move(i_packet));
	}
	int size() const
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		return (int)m_deque.size();
	}
	bool isEmpty() const
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		return m_deque.empty();
	}
	T pop()
	{
		T packet = nullptr;
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		typename std::deque<T>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
			m_deque.erase(it);
		}
		return packet;
	}
	void clear()
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.clear();
	}
	T getFirst()
	{
		T packet = nullptr;
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		typename std::deque<T>::iterator it = m_deque.begin();
		if (it != m_deque.end())
		{
			packet = (*it);
		}
		return packet;
	}
	void eraseFirst()
	{
		std::unique_lock<std::recursive_mutex> slAutoMutexLock(m_mutex);
		m_deque.pop_front();
	}
private:
	std::deque<T> m_deque;
	mutable std::recursive_mutex m_mutex;
};

NS_HJ_END