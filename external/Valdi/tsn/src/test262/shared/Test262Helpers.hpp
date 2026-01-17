#pragma once

#include "valdi_core/cpp/Utils/Format.hpp"
#include <iostream>

#define TEST262_LOG_ARGS(...) , ##__VA_ARGS__
#define TEST262_LOG(_outstream, _format, ...)                                                                          \
    do {                                                                                                               \
        (_outstream) << fmt::format(_format TEST262_LOG_ARGS(__VA_ARGS__)) << std::endl;                               \
    } while (false)

#define TEST262_ERROR(_format, ...) Valdi::Error(fmt::format(_format TEST262_LOG_ARGS(__VA_ARGS__)).c_str())