#pragma once

#include "HJPrerequisites.h"
#include "HJMediaUtils.h"
#include <deque>

NS_HJ_BEGIN

template<typename T>
class HJSlidingWinAvg
{
private:
    std::deque<T> window;
    size_t max_size = 0;
    T sum = T();
public:
    HJSlidingWinAvg(size_t size) : max_size(size) {}
    void add(T value) 
    {
        while (window.size() >= max_size)
        {
            sum -= window.front(); 
            window.pop_front();    
        }
        window.push_back(value);   
        sum += value;              
    }

    T avg() const 
    {   
        return window.empty() ? T() : sum / window.size();
    }

    T avg(T value) const
    {
        if (window.empty())
        {
            return T();
        }
        return (T)((value * sum) / window.size());
    }
    void reset()
    {
        window.clear();
        sum = T();
    }
    
    void updateWinSize(int i_maxSize)
    {
        max_size = i_maxSize;
    }
};

class HJSlidingWinAvgPointf
{
public:
    HJ_DEFINE_CREATE(HJSlidingWinAvgPointf);
    HJSlidingWinAvgPointf()
    {
        priConstruct(10);
    }
    HJSlidingWinAvgPointf(size_t size)
    {
        priConstruct(size);
    }
    
    virtual ~HJSlidingWinAvgPointf() = default;
    
    HJPointf compute(HJPointf i_value)
    {
        m_px->add(i_value.x);
        m_py->add(i_value.y);
        return HJPointf{m_px->avg(), m_py->avg()};
    }
    
    void updateWinSize(int i_nSize)
    {
        m_px->updateWinSize(i_nSize);
        m_py->updateWinSize(i_nSize);
    }
    
    void reset()
    {
        m_px->reset();
        m_py->reset();
    }
private:
    void priConstruct(size_t size)
    {
        m_px = std::make_shared<HJSlidingWinAvg<float>>(size);
        m_py = std::make_shared<HJSlidingWinAvg<float>>(size);
    }
    std::shared_ptr<HJSlidingWinAvg<float>> m_px = nullptr;
    std::shared_ptr<HJSlidingWinAvg<float>> m_py = nullptr;
};

NS_HJ_END