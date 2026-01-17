//
//  DebugLogUtils.h
//  valdi
//
//  Created by Ramzy Jaber on 06/2/22.
//

#pragma once

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

#define _NTH_ARG_GETTER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

#define _invoke_0(_call, _seperator, ...)
#define _invoke_1(_call, _seperator, x) _call(x)
#define _invoke_2(_call, _seperator, x, ...) _call(x) _seperator() _invoke_1(_call, _seperator, __VA_ARGS__)
#define _invoke_3(_call, _seperator, x, ...) _call(x) _seperator() _invoke_2(_call, _seperator, __VA_ARGS__)
#define _invoke_4(_call, _seperator, x, ...) _call(x) _seperator() _invoke_3(_call, _seperator, __VA_ARGS__)
#define _invoke_5(_call, _seperator, x, ...) _call(x) _seperator() _invoke_4(_call, _seperator, __VA_ARGS__)
#define _invoke_6(_call, _seperator, x, ...) _call(x) _seperator() _invoke_5(_call, _seperator, __VA_ARGS__)
#define _invoke_7(_call, _seperator, x, ...) _call(x) _seperator() _invoke_6(_call, _seperator, __VA_ARGS__)
#define _invoke_8(_call, _seperator, x, ...) _call(x) _seperator() _invoke_7(_call, _seperator, __VA_ARGS__)
#define _invoke_9(_call, _seperator, x, ...) _call(x) _seperator() _invoke_8(_call, _seperator, __VA_ARGS__)

#define MACRO_FOR_EACH(x, y, ...)                                                                                      \
    _NTH_ARG_GETTER("",                                                                                                \
                    ##__VA_ARGS__,                                                                                     \
                    _invoke_9,                                                                                         \
                    _invoke_8,                                                                                         \
                    _invoke_7,                                                                                         \
                    _invoke_6,                                                                                         \
                    _invoke_5,                                                                                         \
                    _invoke_4,                                                                                         \
                    _invoke_3,                                                                                         \
                    _invoke_2,                                                                                         \
                    _invoke_1,                                                                                         \
                    _invoke_0)                                                                                         \
    (x, y, ##__VA_ARGS__)

#ifndef _DEBUG_MARKER
#define _DEBUG_MARKER "DLOG"
#endif

#define _DEBUG_FORMAT_SEPERATOR(ignored) " ; "
#define _DEBUG_FORMAT_ELEMENT(ignored) "{} = ({})"
#define _DEBUG_FORMAT_PREAMBLE() _DEBUG_MARKER ":{}:{}: "

#define _DEBUG_ARG_STRINGIFY(s) #s

#define _DEBUG_ARG_SEPERATOR(arg) ,
#define _DEBUG_ARG_ELEMENT(arg) _DEBUG_ARG_STRINGIFY(arg) _DEBUG_ARG_SEPERATOR() arg
#define _DEBUG_ARG_PREAMBLE() __FUNCTION__, __LINE__

#define _DEBUG_FORMAT(...)                                                                                             \
    _DEBUG_FORMAT_PREAMBLE() MACRO_FOR_EACH(_DEBUG_FORMAT_ELEMENT, _DEBUG_FORMAT_SEPERATOR, ##__VA_ARGS__)
#define _DEBUG_ARGS(...)                                                                                               \
    _DEBUG_ARG_PREAMBLE()                                                                                              \
    _DEBUG_ARG_SEPERATOR() MACRO_FOR_EACH(_DEBUG_ARG_ELEMENT, _DEBUG_ARG_SEPERATOR, ##__VA_ARGS__)

#define VALDI_DLOG_VALUES(_logger, ...) VALDI_INFO(_logger, _DEBUG_FORMAT(__VA_ARGS__), _DEBUG_ARGS(__VA_ARGS__))

#define VALDI_DLOG(_logger, format, ...)                                                                               \
    VALDI_INFO(_logger, _DEBUG_FORMAT_PREAMBLE() format, _DEBUG_ARG_PREAMBLE(), ##__VA_ARGS__)
