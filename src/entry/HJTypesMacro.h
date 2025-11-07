#pragma once

#include "HJJson.h"

#define HJ_JSON_AUTO_SERIAL_DESERIAL(...) \
int deserialInfo(const HJYJsonObject::Ptr &obj = nullptr) override { \
HJ_FOR_EACH_APPLY(HJJsonGetValue, __VA_ARGS__) \
return HJ_OK; \
} \
\
int serialInfo(const HJYJsonObject::Ptr& obj = nullptr) override { \
HJ_FOR_EACH_APPLY(HJJsonSetValue, __VA_ARGS__) \
return HJ_OK; \
}

#define HJ_FOR_EACH_APPLY(func, ...) \
HJ_FOR_EACH_APPLY_IMPL(func, __VA_ARGS__)

#define HJ_FOR_EACH_APPLY_IMPL(func, ...) \
HJ_PP_FOREACH(HJ_INVOKE_FUNC, func, __VA_ARGS__)

#define HJ_INVOKE_FUNC(func, var) func(var);

// 主宏定义
#define HJ_PP_FOREACH(macro, data, ...) \
HJ_FOR_EACH(macro, data, __VA_ARGS__)

// 简单的可变参数展开（支持1-7个参数）
#define HJ_FOR_EACH(macro, data, ...) \
HJ_CONCAT(HJ_FOR_EACH_, HJ_VA_ARGS_SIZE(__VA_ARGS__))(macro, data, __VA_ARGS__)

#define HJ_FOR_EACH_1(macro, data, x1) macro(data, x1)
#define HJ_FOR_EACH_2(macro, data, x1, x2) macro(data, x1) macro(data, x2)
#define HJ_FOR_EACH_3(macro, data, x1, x2, x3) macro(data, x1) macro(data, x2) macro(data, x3)
#define HJ_FOR_EACH_4(macro, data, x1, x2, x3, x4) macro(data, x1) macro(data, x2) macro(data, x3) macro(data, x4)
#define HJ_FOR_EACH_5(macro, data, x1, x2, x3, x4, x5) macro(data, x1) macro(data, x2) macro(data, x3) macro(data, x4) macro(data, x5)
#define HJ_FOR_EACH_6(macro, data, x1, x2, x3, x4, x5, x6) macro(data, x1) macro(data, x2) macro(data, x3) macro(data, x4) macro(data, x5) macro(data, x6)
#define HJ_FOR_EACH_7(macro, data, x1, x2, x3, x4, x5, x6, x7) macro(data, x1) macro(data, x2) macro(data, x3) macro(data, x4) macro(data, x5) macro(data, x6) macro(data, x7)
// 参数数量检测宏
#define HJ_VA_ARGS_SIZE(...) HJ_VA_ARGS_SIZE_IMPL(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1)
#define HJ_VA_ARGS_SIZE_IMPL(_1, _2, _3, _4, _5, _6, _7, size, ...) size

// 字符串连接辅助宏
#define HJ_CONCAT(a, b) HJ_CONCAT_IMPL(a, b)
#define HJ_CONCAT_IMPL(a, b) a##b

NS_HJ_BEGIN

class ContextInitConfig final : public HJInterpreter {
public:
    ContextInitConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(valid, logDir, logLevel, logMode, maxSize, maxFiles)

    bool valid;
    std::string logDir;
    int logLevel;
    int logMode;
    int maxSize;
    int maxFiles;
};

class SetWindowConfig final : public HJInterpreter {
public:
    SetWindowConfig(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(surfaceId, width, height, state)

    std::string surfaceId;
    int width = 0;
    int height = 0;
    int state = 0;
};

class MediaStateInfo final : public HJInterpreter {
public:
    MediaStateInfo(HJYJsonObject::Ptr obj) : HJInterpreter(obj) {}

    HJ_JSON_AUTO_SERIAL_DESERIAL(uid, device, sn)

    int64_t uid;
    std::string device;
    std::string sn;
};

NS_HJ_END
