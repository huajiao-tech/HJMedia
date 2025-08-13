
//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJTicker.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJTrajectory::HJTrajectory(const char* file, int line, const char* function, const std::string& info)
    : m_time(0)
    , m_file(file)
    , m_func(function)
    , m_line(line)
    , m_info(info)
{
    m_time = HJTime::getCurrentMicrosecond();
    //
    //m_trace = m_file + "_" + HJ2STR(m_line) + "_" + m_func + "_" + HJ2STR(m_time);
    m_trace = m_file + "(" + m_func + ")";
    m_key = HJUtilitys::hash<std::string>(m_trace);
}

HJTrajectory::~HJTrajectory()
{

}

size_t HJTrajectory::getKey()
{
    return 0;
}

//***********************************************************************************//
std::string HJTracker::formatInfo() {
    std::string info = "trace:{";
    HJTrajectory::Ptr first = m_trajectorys.size() ? m_trajectorys.front() : nullptr;
    HJTrajectory::Ptr pre = nullptr;
    for (auto& cur : m_trajectorys) {
        if (!pre) {
            //info += "[" + cur->getTrace() + "(time:" + HJ2STR(0) + ")" + "] -> ";
            info += HJFMT("[{}(time : {})] -> ", cur->getTrace(), 0);
        }
        else {
            //info += "[" + cur->getTrace() + "(time:" + HJ2STR(cur->m_time - pre->m_time) + ")" + "] -> ";
            info += HJFMT("[{}(time : {})] -> ", cur->getTrace(), cur->m_time - pre->m_time);
        }
        //
        pre = cur;
    }
    if (pre) {
        int64_t time =  pre->m_time - first->m_time;
        info += "end(time : " + HJ2STR(time) + ")}";
    } else {
        info += "}";
    }
    return info;
}
//***********************************************************************************//
NS_HJ_END
