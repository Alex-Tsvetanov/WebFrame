//
//  SCValdiComponentPath.m
//  valdi-ios
//
//  Created by Simon Corsin on 12/19/19.
//

#import "valdi_core/SCValdiComponentPath.h"
#import "valdi_core/SCValdiValueUtils.h"
#import "valdi_core/cpp/Context/ComponentPath.hpp"

FOUNDATION_EXPORT NSString *SCValdiComponentPathGetBundleName(NSString *componentPath) {
    auto componentPathObject = Valdi::ComponentPath::parse(ValdiIOS::StringFromNSString(componentPath));
    return ValdiIOS::NSStringFromString(componentPathObject.getResourceId().bundleName);
}
