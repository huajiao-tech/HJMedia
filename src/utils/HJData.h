#pragma once

#include <atomic>
#include <list>
#include "HJMacros.h"

NS_HJ_BEGIN

struct HJConditionTrigger {
    using Ptr = std::shared_ptr<HJConditionTrigger>;
    using Condition = std::function<bool()>;
    using Callback = std::function<int()>;

    HJConditionTrigger(Condition i_condition, Callback i_callback)
        : m_condition(i_condition)
        , m_callback(i_callback) {}

    Condition m_condition;
    Callback m_callback;
};

template <typename T>
class HJData {
private:
    // 原子类型的成员变量m_data，确保多线程下的安全访问
    std::atomic<T> m_data;
    std::list<HJConditionTrigger::Ptr> m_triggers;

public:
    // 构造函数：初始化原子变量，默认值为T类型的默认值
    HJData() : m_data(T{}) {}

    // 带参数的构造函数：用指定值初始化原子变量
    explicit HJData(const T& i_value) : m_data(i_value) {}

    // 设置数据的方法：使用原子操作store，确保赋值的原子性
    void setData(const T& i_value) {
        m_data.store(i_value);

        for (auto it = m_triggers.begin(); it != m_triggers.end();) {
            auto trigger = *it;
            if (trigger->m_condition() && trigger->m_callback() < 0) {
                it = m_triggers.erase(it);
            }
            else {
                it++;
            }
        }
    }

    // 获取数据的方法：使用原子操作load，确保读取的原子性
    T getData() const {
        return m_data.load();
    }

    // 可选：重载赋值运算符，方便直接赋值
    HJData<T>& operator=(const T& i_value) {
        setData(i_value);
        return *this;
    }

    // 可选：重载类型转换运算符，方便直接获取值
    operator T() const {
        return getData();
    }

    void addTrigger(HJConditionTrigger::Ptr i_trigger) {
        if (i_trigger != nullptr) {
            m_triggers.push_back(i_trigger);
        }
    }
};

NS_HJ_END
