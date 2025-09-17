//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJLog.h"
#include "spdlog/fmt/fmt.h"

//***********************************************************************************//
#define HJFMT(msg, ...) fmt::format(msg, ##__VA_ARGS__)

// use fmt lib, e.g. LOG_WARN("warn log, {1}, {1}, {2}", 1, 2);
#define HJFLogt(msg, ...) HJLogt(HJFMT(msg, ##__VA_ARGS__))
#define HJFLogd(msg, ...) HJLogd(HJFMT(msg, ##__VA_ARGS__))
#define HJFLogi(msg, ...) HJLogi(HJFMT(msg, ##__VA_ARGS__))
#define HJFLogw(msg, ...) HJLogw(HJFMT(msg, ##__VA_ARGS__))
#define HJFLoge(msg, ...) HJLoge(HJFMT(msg, ##__VA_ARGS__))
#define HJFLoga(msg, ...) HJLoga(HJFMT(msg, ##__VA_ARGS__))
#define HJFLogf(msg, ...) HJLogf(HJFMT(msg, ##__VA_ARGS__))

//format name
#define HJFNLogt(msg, ...) HJFLogt(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLogd(msg, ...) HJFLogd(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLogi(msg, ...) HJFLogi(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLogw(msg, ...) HJFLogw(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLoge(msg, ...) HJFLoge(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLoga(msg, ...) HJFLoga(getFMTName() + msg, ##__VA_ARGS__)
#define HJFNLogf(msg, ...) HJFLogf(getFMTName() + msg, ##__VA_ARGS__)


//period log
//#define HJFPERLogi(msg, ...) HJPERIOD_RUN2([&](){HJFLogi(msg, ##__VA_ARGS__);})
#define HJFPERLogi(msg, ...) HJPER2Logi(HJFMT(msg, ##__VA_ARGS__))

#define HJFPERNLogi(msg, ...) HJFPERLogi(getFMTName() + msg, ##__VA_ARGS__)
