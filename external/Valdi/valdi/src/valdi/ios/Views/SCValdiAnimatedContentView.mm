#import "valdi/ios/Views/SCValdiAnimatedContentView.h"

#import "valdi/ios/SnapDrawing/AnimatedImage/SCSnapDrawingAnimatedImageView.h"
#import "valdi_core/SCValdiRuntimeProtocol.h"
#import "valdi_core/SCValdiRuntimeManagerProtocol.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"

#import "valdi/ios/Categories/UIView+Valdi.h"

@interface SCValdiAnimatedContentView ()

@property (nonatomic, readonly) SCSnapDrawingAnimatedImageView *animatedImageView;
@property (nonatomic, readonly, weak) id<SCSnapDrawingRuntime> snapDrawingRuntime;

@end

@implementation SCValdiAnimatedContentView {
    id<SCValdiFunction> _onImageDecodedCallback;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    _animatedImageView.frame = self.bounds;
}

#pragma mark - SCValdiViewComponent

// Currently adding an inner image view in order to grab the drawing runtime from the context. This could be more efficient down the road if we can register this with the global view factory.
- (void)didMoveToValdiContext:(id<SCValdiContextProtocol>)valdiContext
                        viewNode:(id<SCValdiViewNodeProtocol>)viewNode
{
    if (!_animatedImageView) {
        _snapDrawingRuntime = valdiContext.runtime.manager.snapDrawingRuntime;
        _animatedImageView = [[SCSnapDrawingAnimatedImageView alloc] initWithRuntime:_snapDrawingRuntime];
        [self addSubview:_animatedImageView];
    }
}

#pragma mark - SCValdiViewAssetHandlingProtocol

- (void)onValdiAssetDidChange:(nullable id)asset shouldFlip:(BOOL)shouldFlip
{
    SCValdiCppObject *object = ObjectAs(asset, SCValdiCppObject);
    if (object) {
        SCSnapDrawingAnimatedImage *image = [SCSnapDrawingAnimatedImage imageWithCppObject:object];
        [_animatedImageView setImage:image shouldDrawFlipped:shouldFlip];

        if (_onImageDecodedCallback) {
            SCValdiMarshallerScoped(marshaller, {
                SCValdiMarshallerPushDouble(marshaller, image.size.width);
                SCValdiMarshallerPushDouble(marshaller, image.size.height);

                [_onImageDecodedCallback performWithMarshaller:marshaller];
            });
        }
    } else {
        [_animatedImageView clearImage];
    }
}

#pragma mark - UIView+Valdi

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAssetAttributesForOutputType:SCNValdiCoreAssetOutputTypeLottie];
    [attributesBinder bindAttribute:@"advanceRate" invalidateLayoutOnChange:NO withDoubleBlock:^BOOL(SCValdiAnimatedContentView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAdvanceRate:attributeValue];
        return YES;
    } resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAdvanceRate:0];
    }];
    [attributesBinder bindAttribute:@"currentTime" invalidateLayoutOnChange:NO withDoubleBlock:^BOOL(SCValdiAnimatedContentView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setCurrentTime:attributeValue];
        return YES;
    } resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setCurrentTime:0];
    }];
    [attributesBinder bindAttribute:@"animationStartTime" invalidateLayoutOnChange:NO withDoubleBlock:^BOOL(SCValdiAnimatedContentView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAnimationStartTime:attributeValue];
        return YES;
    } resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAnimationStartTime:0];
    }];

    [attributesBinder bindAttribute:@"animationEndTime" invalidateLayoutOnChange:NO withDoubleBlock:^BOOL(SCValdiAnimatedContentView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAnimationEndTime:attributeValue];
        return YES;
    } resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setAnimationEndTime:0];
    }];
    [attributesBinder bindAttribute:@"loop" invalidateLayoutOnChange:NO withBoolBlock:^BOOL(SCValdiAnimatedContentView *view, BOOL attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setShouldLoop:attributeValue];
        return YES;
    } resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setShouldLoop:NO];
    }];
    [attributesBinder bindAttribute:@"onProgress" withFunctionBlock:^(SCValdiAnimatedContentView *view, id<SCValdiFunction> attributeValue) {
        [view.animatedImageView setProgressHandler:^(NSTimeInterval currentTime, NSTimeInterval duration) {
            SCValdiMarshallerScoped(marshaller, {
                
                NSInteger objectIndex = SCValdiMarshallerPushMap(marshaller, 1);
                SCValdiMarshallerPushDouble(marshaller, currentTime);
                SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"time", objectIndex);
                SCValdiMarshallerPushDouble(marshaller, duration);
                SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"duration", objectIndex);
                
                [attributeValue performWithMarshaller:marshaller];
            });
        }];
    } resetBlock:^(SCValdiAnimatedContentView *view) {
        [view.animatedImageView setProgressHandler:nil];
    }];

    [attributesBinder bindAttribute:@"objectFit" invalidateLayoutOnChange:NO
        withStringBlock:^BOOL(SCValdiAnimatedContentView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view.animatedImageView setObjectFit:attributeValue];
            return YES;
    }
    resetBlock:^(SCValdiAnimatedContentView *view, id<SCValdiAnimatorProtocol> animator) {
        [view.animatedImageView setObjectFit:nil];
    }];

    [attributesBinder bindAttribute:@"onImageDecoded"
        withFunctionBlock: ^(SCValdiAnimatedContentView *view, id<SCValdiFunction> attributeValue) {
            [view valdi_setOnImageDecodedCallback:attributeValue];
    }
    resetBlock:^(SCValdiAnimatedContentView *view) {
        [view valdi_setOnImageDecodedCallback:nil];
    }];
}

- (BOOL)valdi_setOnImageDecodedCallback: (id<SCValdiFunction>)attributeValue
{
    _onImageDecodedCallback = attributeValue;

    return YES;
}

@end
