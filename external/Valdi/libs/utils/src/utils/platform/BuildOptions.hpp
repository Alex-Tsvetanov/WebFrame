#pragma once

#if __has_include("build_options/BuildOptionsGenerated.hpp")
#include "build_options/BuildOptionsGenerated.hpp"
#else
// Fallback for cmake based builds, we keep all flags enabled
// by default in this scenario.
#include "utils/platform/BuildOptionsDefault.hpp"
#endif
