//
//  SCValdiValueUtils.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/25/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/cpp/Utils/StringBox.hpp"
#import <Foundation/Foundation.h>

namespace ValdiIOS {

// Implementation lives in SCValdiObjCConversionUtils
// Cleanup once Objective-C++ code has been fully removed from iOS

Valdi::StringBox StringFromNSString(__unsafe_unretained NSString* nsString);
Valdi::StringBox InternedStringFromNSString(__unsafe_unretained NSString* nsString);
std::string_view StringViewFromNSString(__unsafe_unretained NSString* nsString);
NSString* NSStringFromString(const Valdi::StringBox& stringBox);
NSString* NSStringFromSTDStringView(const std::string_view& stdString);

} // namespace ValdiIOS
