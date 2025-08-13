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

