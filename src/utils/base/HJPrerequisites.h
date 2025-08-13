#pragma once

#include <string>
#include <mutex>
#include "HJMacros.h"

#define HJ_LOCK(name) std::lock_guard<std::mutex> lock(name);

#define HJ_DEFINE_CREATE(ClassType)             \
	using Ptr = std::shared_ptr<ClassType>;     \
    using Wtr = std::weak_ptr<ClassType>; \
    static std::shared_ptr<ClassType> Create(bool i_bUseWeak = false) {      \
        if (i_bUseWeak) \
        { \
            return std::shared_ptr<ClassType>(new ClassType()); \
        } \
        else \
        { \
            return std::make_shared<ClassType>();         \
        } \
    } \
    static Ptr GetPtrFromWtr(Wtr i_wr) { \
        Ptr ptr = nullptr; \
        do \
        { \
	        if (!i_wr.expired()) \
	        { \
		        ptr = i_wr.lock(); \
		        if (!ptr) \
		        { \
			        break;\
		        } \
	        } \
	        else \
	        { \
		        break; \
	        } \
        } while (false);\
        return ptr;\
    }\
