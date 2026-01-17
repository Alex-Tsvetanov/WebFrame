//
//  SCValdiImage.m
//  valdi-ios
//
//  Created by Simon Corsin on 9/25/19.
//

#import "valdi_core/SCValdiImage.h"
#import "valdi_core/UIImage+Valdi.h"

@implementation SCValdiImage {
    UIImage *_image;
}

- (instancetype)initWithImage:(UIImage *)image
{
    self = [super init];

    if (self) {
        _image = image;
    }

    return self;
}

- (NSData*)toPNG
{
    UIImage *image = [self UIImageRepresentation];
    if (!image) {
        return nil;
    }

    return UIImagePNGRepresentation(image);
}

- (UIImage *)UIImageRepresentation
{
    return _image;
}

- (CGSize)size
{
    return _image.size;
}

+ (instancetype)imageWithModuleName:(NSString *)moduleName resourcePath:(NSString *)resourcePath
{
    NSString *correctedResourcePath = [resourcePath stringByReplacingOccurrencesOfString:@"-" withString:@"_"];

    // We explicitly allocate those UIImages to warm them up.
    // SCValdiImage are going to be cached within the AssetManager.

    UIImage *image = [UIImage imageNamed:correctedResourcePath inValdiBundle:moduleName];
    if (image) {
        return [[self alloc] initWithImage:image];
    }

    image = [UIImage imageNamed:resourcePath];
    if (image) {
        return [[self alloc] initWithImage:image];
    }

    return nil;
}

+ (nullable instancetype)imageWithFilePath:(NSString *)path error:(NSError **)error
{
    UIImage *image = [UIImage imageWithContentsOfFile:path];
    if (!image) {
        if (error) {
            // Attempt to figure out the error for debugging
            if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
                *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedFailureReasonErrorKey: @"File does not exist"}];
            } else {
                *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedFailureReasonErrorKey: @"imageWithContentsOfFile: returned nil"}];
            }
        }
        return nil;
    }

    return [[self alloc] initWithImage:image];
}

+ (nullable instancetype)imageWithData:(NSData *)imageData error:(NSError *__autoreleasing  _Nullable *)error
{
    UIImage *image = [UIImage imageWithData:imageData];
    if (!image) {
        *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedFailureReasonErrorKey: @"Cannot load image from data"}];
        return nil;
    }

    return [self imageWithUIImage:image];
}

+ (nullable instancetype)imageWithUIImage:(nullable UIImage *)image
{
    if (!image) {
        return nil;
    }

    return [[self alloc] initWithImage:image];
}

+ (NSString *)urlStringForBundleName:(NSString *)bundleName imageName:(NSString *)imageName
{
    if (!bundleName) {
        return [NSString stringWithFormat:@"valdi-res://%@", imageName];
    } else {
        return [NSString stringWithFormat:@"valdi-res://%@/%@", bundleName, imageName];
    }
}

@end
