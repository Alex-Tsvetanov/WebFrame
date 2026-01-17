//
//  SCValdiImage.h
//  valdi-ios
//
//  Created by Simon Corsin on 9/25/19.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiImage : NSObject

@property (readonly, nonatomic) CGSize size;

- (UIImage*)UIImageRepresentation;
- (NSData*)toPNG;

+ (nullable instancetype)imageWithModuleName:(NSString*)moduleName resourcePath:(NSString*)resourcePath;
+ (nullable instancetype)imageWithFilePath:(NSString*)path error:(NSError**)error;
+ (nullable instancetype)imageWithData:(NSData*)imageData error:(NSError**)error;
+ (nullable instancetype)imageWithUIImage:(nullable UIImage*)image;

/**
 * Returns an NSString representing an image URL that points to the given "imageName" in the NSBundle
 * with the given "bundleName". If "bundleName" is nil, only "imageName" will be used using the
 * +[UIImage imageWithName:] API.
 * The returned string can then be used as a "src" attribute for an "image" element in TypeScript.
 */
+ (NSString*)urlStringForBundleName:(nullable NSString*)bundleName imageName:(NSString*)imageName;

@end

NS_ASSUME_NONNULL_END
