#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImageView.h"
#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImage+CPP.h"
#import "valdi/ios/SnapDrawing/SCSnapDrawingUIView+CPP.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"

#include "valdi/snap_drawing/Layers/Classes/AnimatedImageLayerClass.hpp"
#include "snap_drawing/cpp/Layers/AnimatedImageLayer.hpp"

class ObjCAnimatedImageLayerListener: public snap::drawing::AnimatedImageLayerListener {
public:
    ObjCAnimatedImageLayerListener(SCSnapDrawingAnimatedImageProgressHandler progressHandler): _progressHandler([progressHandler copy]) {}
    ~ObjCAnimatedImageLayerListener() = default;

    void onProgress(snap::drawing::AnimatedImageLayer& animatedImageLayer, const snap::drawing::Duration& time, const snap::drawing::Duration& duration) override {
        _progressHandler(time.seconds(), duration.seconds());
    }
private:
    SCSnapDrawingAnimatedImageProgressHandler _progressHandler;
};

@implementation SCSnapDrawingAnimatedImageView {
    Valdi::Ref<snap::drawing::AnimatedImageLayer> _animatedImageLayer;
}

- (instancetype)initWithRuntime:(id<SCSnapDrawingRuntime>)runtime
{
    self = [super initWithRuntime:runtime];
    if (self) {
        auto cppRuntime = [self cppRuntime];

        _animatedImageLayer = snap::drawing::makeLayer<snap::drawing::AnimatedImageLayer>(cppRuntime->getResources());
        self.layerRoot->setContentLayer(_animatedImageLayer, snap::drawing::ContentLayerSizingModeMatchSize);
    }
    return self;
}

- (void)setImage:(SCSnapDrawingAnimatedImage *)image shouldDrawFlipped:(BOOL)shouldDrawFlipped
{
    _animatedImageLayer->setImage(image.animatedImage);
    _animatedImageLayer->setShouldFlip(shouldDrawFlipped);
}

- (void)clearImage {
    _animatedImageLayer->setImage(nullptr);
}

- (void)setAdvanceRate:(double)advanceRate
{
    _animatedImageLayer->setAdvanceRate(advanceRate);
}

- (void)setShouldLoop:(BOOL)shouldLoop
{
    _animatedImageLayer->setShouldLoop(shouldLoop);
}

- (void)setCurrentTime:(NSTimeInterval)currentTime
{
    _animatedImageLayer->setCurrentTime(snap::drawing::Duration::fromSeconds(currentTime));
}

- (void)setAnimationStartTime:(NSTimeInterval)startTime
{
    _animatedImageLayer->setAnimationStartTime(snap::drawing::Duration::fromSeconds(startTime));
}

- (void)setAnimationEndTime:(NSTimeInterval)endTime
{
    _animatedImageLayer->setAnimationEndTime(snap::drawing::Duration::fromSeconds(endTime));
}

- (void)setProgressHandler:(nullable SCSnapDrawingAnimatedImageProgressHandler)progressHandler
{
    if (progressHandler) {
        auto listener = Valdi::makeShared<ObjCAnimatedImageLayerListener>(progressHandler);
        _animatedImageLayer->setListener(listener);
    } else {
        _animatedImageLayer->setListener(nullptr);
    }
}

- (void)setObjectFit:(nullable NSString *)objectFit
{
    snap::drawing::FittingSizeMode fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit;

    if ([objectFit isEqualToString:@"none"]) {
        fittingSizeMode = snap::drawing::FittingSizeModeCenter;
    } else if ([objectFit isEqualToString:@"fill"]) {
        fittingSizeMode = snap::drawing::FittingSizeModeFill;
    } else if ([objectFit isEqualToString:@"cover"]) {
        fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFill;
    } else if ([objectFit isEqualToString:@"contain"]) {
        fittingSizeMode = snap::drawing::FittingSizeModeCenterScaleFit;
    }

    _animatedImageLayer->setFittingSizeMode(fittingSizeMode);
}

@end
