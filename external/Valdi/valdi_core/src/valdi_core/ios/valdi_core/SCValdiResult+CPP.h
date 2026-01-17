//
//  SCValdiResult+CPP.h
//  valdi
//
//  Created by Simon Corsin on 9/5/19.
//

#pragma once

#import "valdi_core/SCValdiResult.h"
#import "valdi_core/cpp/Utils/PlatformResult.hpp"

NS_ASSUME_NONNULL_BEGIN

SCValdiResult* SCValdiResultFromPlatformResult(const Valdi::PlatformResult& platformResult);
Valdi::PlatformResult SCValdiResultToPlatformResult(__unsafe_unretained SCValdiResult* result);

NS_ASSUME_NONNULL_END
