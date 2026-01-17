//
//  SCValdiImageFilter.m
//  valdi-ios
//
//  Created by Simon Corsin on 5/2/22.
//

#import "SCValdiImageFilter.h"
#import <CoreImage/CoreImage.h>
#import "valdi_core/cpp/Threading/DispatchQueue.hpp"

namespace ValdiIOS {

static CIFilter *makeBlurFilter(const Valdi::Ref<Valdi::ImageFilter> &filter) {
    if (filter->getBlurRadius() == 0.0f) {
        return nil;
    }

    return [CIFilter filterWithName:@"CIGaussianBlur" keysAndValues:@"inputRadius", @(filter->getBlurRadius()), nil];
}

static CIVector *makeCIVector(const Valdi::Ref<Valdi::ImageFilter> &filter, size_t index) {
    return [CIVector vectorWithX:filter->getColorMatrixComponent(index)
                               Y:filter->getColorMatrixComponent(index + 1)
                               Z:filter->getColorMatrixComponent(index + 2)
                               W:filter->getColorMatrixComponent(index + 3)];
}

static CIFilter *makeColorMatrixFilter(const Valdi::Ref<Valdi::ImageFilter> &filter) {
    CIVector *inputRVector = makeCIVector(filter, 0);
    CIVector *inputGVector = makeCIVector(filter, 5);
    CIVector *inputBVector = makeCIVector(filter, 10);
    CIVector *inputAVector = makeCIVector(filter, 15);
    CIVector *inputBiasVector = [CIVector vectorWithX:filter->getColorMatrixComponent(4)
                                                    Y:filter->getColorMatrixComponent(9)
                                                    Z:filter->getColorMatrixComponent(14)
                                                    W:filter->getColorMatrixComponent(19)];

    return [CIFilter filterWithName:@"CIColorMatrix" keysAndValues:
                 @"inputRVector", inputRVector,
                 @"inputGVector", inputGVector,
                 @"inputBVector", inputBVector,
                 @"inputAVector", inputAVector,
                 @"inputBiasVector", inputBiasVector,
                 nil];
}

static CIImage *applyFilter(CIImage *image, CIFilter *filter) {
    if (filter) {
        [filter setValue:image forKey:kCIInputImageKey];
        return [filter valueForKey:kCIOutputImageKey];
    } else {
        return image;
    }
}

static UIImage *rasterizeImage(CIImage *image,
                               CGRect rect,
                               CGFloat scale,
                               UIImageOrientation orientation,
                               UIImageRenderingMode renderingMode,
                               CIContext *providedContext) {
    static CIContext *localContext;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        localContext = [CIContext context];
    });

    static CIContext *context;
    context = providedContext ? providedContext : localContext;
    CGImageRef cgImage = [context createCGImage:image fromRect:rect];
    UIImage *outputImage = [UIImage imageWithCGImage:cgImage scale:scale orientation:orientation];
    CGImageRelease(cgImage);

    if (renderingMode != outputImage.renderingMode) {
        outputImage = [outputImage imageWithRenderingMode:renderingMode];
    }

    // For memory savings, clearCache should be called when cached images are 
    // likely no longer in use. Check to see if the last call to rasterizeImage
    // was over clearThreshold ms ago and if it was, then clear the cache.
    // 
    // If it's not over the threshold, then calls to rasterizeImage are still
    // being made and the cache should be kept in case it is needed.
    static constexpr std::chrono::milliseconds clearThreshold(1000); 
    static auto lastCallTime = std::chrono::steady_clock::time_point();
    lastCallTime = std::chrono::steady_clock::now();

    Valdi::DispatchFunction dispatchFn = []() {
        auto currentTime = std::chrono::steady_clock::now();
        if ((currentTime - lastCallTime) >= clearThreshold)
            [context clearCaches];
        };
    
    Valdi::DispatchQueue::getCurrent()->asyncAfter(dispatchFn, clearThreshold);

    return outputImage;
}

SCValdiImage *postprocessImage(SCValdiImage *input,
                                         const Valdi::Ref<Valdi::ImageFilter> &filter) {
    return postprocessImage(input, filter, nullptr);
}

SCValdiImage *postprocessImage(SCValdiImage *input,
                                         const Valdi::Ref<Valdi::ImageFilter> &filter,
                                         CIContext* context) {
    UIImage *uiImage = input.UIImageRepresentation;
    if (!uiImage) {
        return input;
    }

    CIImage *ciImage = nil;

    if (uiImage.CIImage != nil) {
        ciImage = uiImage.CIImage;
    } else {
        ciImage = [CIImage imageWithCGImage:uiImage.CGImage];
    }

    CGRect rect = ciImage.extent;
    ciImage = [ciImage imageByClampingToExtent];

    ciImage = applyFilter(ciImage, makeColorMatrixFilter(filter));
    ciImage = applyFilter(ciImage, makeBlurFilter(filter));

    UIImage *outputImage = rasterizeImage(ciImage,
                                          rect,
                                          uiImage.scale,
                                          uiImage.imageOrientation,
                                          uiImage.renderingMode,
                                          context);

    return [SCValdiImage imageWithUIImage:outputImage];
}
}
