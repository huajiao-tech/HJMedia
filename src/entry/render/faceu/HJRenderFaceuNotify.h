#pragma once

#include "HJNotify.h"
#include <functional>

using HJFaceuListener = std::function<int(const std::string, const HJ::HJNotification::Ptr)>;






