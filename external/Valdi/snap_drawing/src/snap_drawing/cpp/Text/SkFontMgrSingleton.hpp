#pragma once

#include "include/core/SkFontMgr.h"

namespace snap::snap_drawing {

sk_sp<SkFontMgr> getSkFontMgrSingleton();

}