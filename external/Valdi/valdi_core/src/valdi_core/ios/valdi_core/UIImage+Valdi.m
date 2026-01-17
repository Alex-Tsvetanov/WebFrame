//
//  UIImage+Valdi.m
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi_core/UIImage+Valdi.h"

#import "valdi_core/SCValdiRuntimeProtocol.h"

@implementation UIImage (Valdi)

static NSMutableDictionary<NSString *, NSBundle *> *_SCValdiBundleCache()
{
    static dispatch_once_t onceToken;
    __strong static NSMutableDictionary<NSString *, NSBundle *> *bundleCache = nil;
    dispatch_once(&onceToken, ^{
        bundleCache = [[NSMutableDictionary alloc] init];
    });
    return bundleCache;
}

+ (UIImage *)imageFromValdiAttributeValue:(id)attributeValue context:(id<SCValdiContextProtocol>)context
{
    if ([attributeValue isKindOfClass:[UIImage class]]) {
        return attributeValue;
    }
    if ([attributeValue isKindOfClass:[NSString class]]) {
        if (context) {
            NSString *bundleName;
            NSString *imageName = (NSString *)attributeValue;
            NSRange rangeOfSemicolon = [imageName rangeOfString:@":"];
            if (rangeOfSemicolon.location != NSNotFound) {
                // Split the bundle name apart from the image name
                bundleName = [imageName substringToIndex:rangeOfSemicolon.location];
                attributeValue = [imageName substringFromIndex:(rangeOfSemicolon.location + 1)];
            }

            if (bundleName) {
                UIImage *image = [self imageNamed:attributeValue inValdiBundle:bundleName];
                if (image) {
                    return image;
                }
            }
        }

        // Fallback to image named
        if (((NSString *)attributeValue).length > 0) {
            return [UIImage imageNamed:attributeValue];
        }
    }

    return nil;
}

+ (UIImage *)imageNamed:(NSString *)imageName inValdiBundle:(NSString *)bundleName
{
    NSString *cleanedImageName = [imageName stringByReplacingOccurrencesOfString:@"-" withString:@"_"];

    NSMutableDictionary<NSString *, NSBundle *> *cache = _SCValdiBundleCache();
    NSString *cacheKey = [bundleName copy];

    NSBundle *bundle;
    @synchronized (cache) {
        bundle = [cache objectForKey:cacheKey];
        if (!bundle) {
            bundle =
            [NSBundle bundleWithPath:[NSString stringWithFormat:@"%@/%@.bundle", [[NSBundle mainBundle] bundlePath],
                                      bundleName]];
            if (!bundle) {
                return nil;
            }
            [cache setObject:bundle forKey:cacheKey];
        }

        return [UIImage imageNamed:cleanedImageName inBundle:bundle compatibleWithTraitCollection:nil];
    }
}

@end
