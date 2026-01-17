//
//  SCValdiResourceLoader.m
//  valdi-ios
//
//  Created by Simon Corsin on 10/1/19.
//

#import <Foundation/Foundation.h>
#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi/ios/Resources/SCValdiResourceLoader.h"
#import "valdi_core/cpp/Utils/DiskUtils.hpp"
#import "valdi_core/cpp/Utils/PathUtils.hpp"
#import "valdi_core/SCValdiImage.h"
#import "valdi_core/cpp/Utils/Format.hpp"
#import "valdi_core/SCValdiCustomModuleProvider.h"

namespace ValdiIOS {

static NSURL *getResourceUrlForResourceName(NSString *resourceName) {
    NSURL *url = [[NSBundle mainBundle] URLForResource:resourceName withExtension:nil];
    if (url) {
        return url;
    }

    url = [[NSBundle bundleForClass:SCValdiImage.class] URLForResource:resourceName withExtension:nil];
    if (url) {
        return url;
    }

    // Otherwise lookup to all the other bundles
    for (NSBundle *bundle in [NSBundle allBundles]) {
        url = [bundle URLForResource:resourceName withExtension:nil];
        if (url) {
            return url;
        }
    }

    return nil;
}

ResourceLoader::ResourceLoader(id<SCValdiCustomModuleProvider> moduleProvider): _moduleProvider(moduleProvider) {}

ResourceLoader::~ResourceLoader() = default;

Valdi::Result<Valdi::BytesView> ResourceLoader::loadModuleContent(const Valdi::StringBox &module) {
    NSString *resourceName = ValdiIOS::NSStringFromString(module);
    if (_moduleProvider) {
        NSError *error = nil;
        NSData *data = [_moduleProvider customModuleDataForPath:resourceName error:&error];
        if (data) {
            return ValdiIOS::BufferFromNSData(data);
        }

        if (error) {
            return Valdi::Error(ValdiIOS::InternedStringFromNSString(error.localizedDescription));
        }
    }

    NSURL *url = getResourceUrlForResourceName(resourceName);
    if (!url) {
        return Valdi::Error("Could not find module");
    }

    auto cppPath = ValdiIOS::StringFromNSString(url.path);
    Valdi::Path path(cppPath.toStringView());

    return Valdi::DiskUtils::load(path);
}

Valdi::StringBox ResourceLoader::resolveLocalAssetURL(const Valdi::StringBox &moduleName, const Valdi::StringBox &resourcePath) {
    NSString *objcModuleName = ValdiIOS::NSStringFromString(moduleName);
    NSString *objcResourcePath = ValdiIOS::NSStringFromString(resourcePath);

    SCValdiImage *image = [SCValdiImage imageWithModuleName:objcModuleName resourcePath:objcResourcePath];
    if (!image) {
        return Valdi::StringBox();
    }

    return STRING_FORMAT("valdi-res://{}/{}", moduleName, resourcePath);
}

}
