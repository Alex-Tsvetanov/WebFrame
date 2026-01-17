//
//  SCValdiImageView.m
//  Valdi
//
//  Created by Simon Corsin on 2/20/19.
//

#import "valdi_core/SCValdiImageView.h"

#import "valdi_core/NSURL+Valdi.h"
#import "valdi_core/SCValdiActions.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/SCValdiJSConversionUtils.h"
#import "valdi_core/SCValdiImage.h"
#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCNValdiCoreAssetLoadObserver.h"
#import "valdi_core/UIColor+Valdi.h"
#import "valdi_core/UIImage+Valdi.h"
#import "valdi_core/UIView+ValdiObjects.h"
#import "valdi_core/UIView+ValdiBase.h"

#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiAttributesBinderBase.h"
#import "valdi_core/SCNValdiCoreCompositeAttributePart.h"
#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiRuntimeProtocol.h"

#import "valdi_core/SCValdiAction.h"

@interface SCValdiImageViewInner: UIImageView

@property (readonly, nonatomic) BOOL shouldFlip;

@end

@implementation SCValdiImageViewInner {
    SCValdiImage *_valdiImage;
    BOOL _hasTintColor;
}

- (BOOL)setValdiImage:(SCValdiImage *)valdiImage shouldFlip:(BOOL)shouldFlip
{
    BOOL imageNeedsUpdate = NO;
    if (_valdiImage != valdiImage) {
        _valdiImage = valdiImage;
        imageNeedsUpdate = YES;
    }

    if (_shouldFlip != shouldFlip) {
        _shouldFlip = shouldFlip;
        imageNeedsUpdate = YES;
    }

    if (imageNeedsUpdate) {
        [self _updateImage];
    }
    return imageNeedsUpdate;
}

- (void)setTintColor:(UIColor *)tintColor
{
    [super setTintColor:tintColor];

    BOOL hasTintColor = tintColor != nil;
    if (_hasTintColor != hasTintColor) {
        _hasTintColor = hasTintColor;
        [self _updateImage];
    }
}

- (void)_updateImage
{
    UIImage *finalImage = _valdiImage.UIImageRepresentation;

    if (_shouldFlip) {
        finalImage = [finalImage imageWithHorizontallyFlippedOrientation];
    }

    if (_hasTintColor) {
        finalImage = [finalImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }

    self.image = finalImage;
}

@end

@interface SCValdiImageView() <SCNValdiCoreAssetLoadObserver>

@end

@implementation SCValdiImageView {
    SCNValdiCoreAsset *_currentAsset;
    BOOL _observingAsset;
    SCValdiImageViewInner *_imageView;
    CGFloat _contentScaleX;
    CGFloat _contentScaleY;
    CGFloat _contentRotation;
    id<SCValdiFunction> _onImageDecodedCallback;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];

    if (self) {
        _imageView = [[SCValdiImageViewInner alloc] initWithImage:nil];
        _imageView.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:_imageView];
        _contentScaleX = 1;
        _contentScaleY = 1;
        _contentRotation = 0;
        self.clipsToBounds = YES;
    }

    return self;
}

- (void)onValdiAssetDidChange:(nullable id)asset shouldFlip:(BOOL)shouldFlip
{
    SCValdiImage *image = ObjectAs(asset, SCValdiImage);
    BOOL needsUpdate = [_imageView setValdiImage:image shouldFlip:shouldFlip];

    if (_onImageDecodedCallback && needsUpdate) {
            SCValdiMarshallerScoped(marshaller, {
                SCValdiMarshallerPushDouble(marshaller, image.size.width);
                SCValdiMarshallerPushDouble(marshaller, image.size.height);
                
                [_onImageDecodedCallback performWithMarshaller:marshaller];
            });
        }
}

- (void)onLoad:(SCNValdiCoreAsset *)asset loadedAsset:(NSObject *)loadedAsset error:(NSString *)error
{
    if (asset != _currentAsset) {
        return;
    }

    [self onValdiAssetDidChange:loadedAsset shouldFlip:_imageView.shouldFlip];
}

- (void)_applyAsset:(SCNValdiCoreAsset *)asset shouldFlip:(BOOL)shouldFlip
{
    if (_currentAsset != asset) {
        SCNValdiCoreAsset *previousAsset = _currentAsset;
        _currentAsset = asset;

        if (_observingAsset) {
            _observingAsset = NO;
            [previousAsset removeLoadObserver:self];
        }

        [self onValdiAssetDidChange:nil shouldFlip:shouldFlip];

        if (_currentAsset) {
            _observingAsset = YES;
            [_currentAsset addLoadObserver:self outputType:SCNValdiCoreAssetOutputTypeImageIOS preferredWidth:0 preferredHeight:0 filter:[NSNull null]];
        }
    }
}

- (BOOL)valdi_setObjectFit:(NSString *)attributeValue
{
    if (!attributeValue) {
        _imageView.contentMode = UIViewContentModeScaleToFill;
        return YES;
    }

    NSString *contentModeString = attributeValue;
    UIViewContentMode contentMode;
    if ([contentModeString isEqualToString:@"none"]) {
        contentMode = UIViewContentModeCenter;
    } else if ([contentModeString isEqualToString:@"fill"]) {
        contentMode = UIViewContentModeScaleToFill;
    } else if ([contentModeString isEqualToString:@"cover"]) {
        contentMode = UIViewContentModeScaleAspectFill;
    } else if ([contentModeString isEqualToString:@"contain"]) {
        contentMode = UIViewContentModeScaleAspectFit;
    } else {
        return NO;
    }

    _imageView.contentMode = contentMode;

    return YES;
}

- (BOOL)valdi_setImageTintColor:(UIColor *)tintColor
{
    _imageView.tintColor = tintColor;

    return YES;
}

- (void)setAsset:(SCNValdiCoreAsset *)asset
       tintColor:(UIColor *)tintColor
       flipOnRtl:(BOOL)flipOnRtl
{
    [self valdi_setImageTintColor:tintColor];
    BOOL shouldFlip = flipOnRtl ? self.effectiveUserInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft : NO;
    [self _applyAsset:asset shouldFlip:shouldFlip];
}

- (BOOL)valdi_setContentTransform:(id)attributeValue animator:(id<SCValdiAnimatorProtocol>)animator
{
    if (attributeValue != nil) {
        NSArray *parts = ObjectAs(attributeValue, NSArray);
        if (parts.count != 3) {
            return NO;
        }
        _contentScaleX = (ObjectAs(parts[0], NSNumber) ?: @(1.0)).doubleValue;
        _contentScaleY = (ObjectAs(parts[1], NSNumber) ?: @(1.0)).doubleValue;
        _contentRotation = (ObjectAs(parts[2], NSNumber) ?: @(0.0)).doubleValue;
    } else {
        _contentScaleX = 1;
        _contentScaleY = 1;
        _contentRotation = 0;
    }

    [self applyContentTransform];

    return YES;
}

- (void)valdi_applySlowClipping:(BOOL)slowClipping animator:(id<SCValdiAnimatorProtocol> )animator
{
    self.clipsToBounds = slowClipping;
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAssetAttributesForOutputType:SCNValdiCoreAssetOutputTypeImageIOS];

    [attributesBinder bindAttribute:@"tint" invalidateLayoutOnChange:NO withColorBlock:^BOOL(SCValdiImageView *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
        return [view valdi_setImageTintColor:attributeValue];
    } resetBlock:^(SCValdiImageView *view, id<SCValdiAnimatorProtocol> animator) {
        [view valdi_setImageTintColor:nil];
    }];

    SCNValdiCoreCompositeAttributePart *contentScaleXAttribute =
        [[SCNValdiCoreCompositeAttributePart alloc] initWithAttribute:@"contentScaleX"
                                                                type:SCNValdiCoreAttributeTypeDouble
                                                            optional:YES
                                            invalidateLayoutOnChange:NO];
    SCNValdiCoreCompositeAttributePart *contentScaleYAttribute =
        [[SCNValdiCoreCompositeAttributePart alloc] initWithAttribute:@"contentScaleY"
                                                                type:SCNValdiCoreAttributeTypeDouble
                                                            optional:YES
                                            invalidateLayoutOnChange:NO];

    SCNValdiCoreCompositeAttributePart *contentRotationAttribute =
        [[SCNValdiCoreCompositeAttributePart alloc] initWithAttribute:@"contentRotation"
                                                                type:SCNValdiCoreAttributeTypeDouble
                                                            optional:YES
                                            invalidateLayoutOnChange:NO];

    [attributesBinder bindCompositeAttribute:@"contentTransform"
        parts:@[
            contentScaleXAttribute,
            contentScaleYAttribute,
            contentRotationAttribute,
        ]
        withUntypedBlock:^BOOL(SCValdiImageView *view, id attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setContentTransform:attributeValue animator:animator];
        }
        resetBlock:^(SCValdiImageView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setContentTransform:nil animator:animator];
        }];

    [attributesBinder bindAttribute:@"objectFit"
        invalidateLayoutOnChange:NO
        withStringBlock:^BOOL(SCValdiImageView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setObjectFit:attributeValue];
        }
        resetBlock:^(SCValdiImageView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setObjectFit:nil];
        }];

    [attributesBinder bindAttribute:@"onImageDecoded"
        withFunctionBlock:^(SCValdiImageView *view, id<SCValdiFunction> attributeValue) {
            [view valdi_setOnImageDecodedCallback:attributeValue];
        }
        resetBlock:^(SCValdiImageView *view) {
            [view valdi_setOnImageDecodedCallback:nil];
        }];
}

- (BOOL)valdi_setOnImageDecodedCallback:(id<SCValdiFunction>)attributeValue
{
    _onImageDecodedCallback = attributeValue;

    return YES;
}

- (BOOL)willEnqueueIntoValdiPool
{
    return self.class == [SCValdiImageView class];
}

- (BOOL)clipsToBoundsByDefault
{
    return YES;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self applyContentTransform];
}

- (void)applyContentTransform
{
    CGFloat parentWidth = self.bounds.size.width;
    CGFloat parentHeight = self.bounds.size.height;
    CGFloat childWidth = parentWidth * _contentScaleX;
    CGFloat childHeight = parentHeight * _contentScaleY;
    CGFloat diffWidth = childWidth - parentWidth;
    CGFloat diffHeight = childHeight - parentHeight;

    _imageView.bounds = CGRectMake(0, 0, childWidth, childHeight);
    _imageView.center = CGPointMake(-diffWidth / 2 + childWidth / 2, -diffHeight / 2 + childHeight / 2);

    CGAffineTransform transform = CGAffineTransformIdentity;

    // The bounds are used to scale the image to the correct size, however, if the contentScaleX or contentScaleY
    // are a negative value we still need to apply a transform to invert the image on the correct axis.
    if (_contentScaleX < 0.0 || _contentScaleY < 0.0) {
        transform = CGAffineTransformScale(
            transform,
            // if contentScaleX is negative, flip the image, the scale amount is done with the bounds
            _contentScaleX < 0.0 ? -1.0 : 1.0,
            // if contentScaleY is negative, flip the image, the scale amunt is done with the bounds
            _contentScaleY < 0.0 ? -1.0 : 1.0
        );
    }

    if (_contentRotation != 0) {
        transform = CGAffineTransformRotate(transform, _contentRotation);
    }
    _imageView.transform = transform;
}

- (CGSize)sizeThatFits:(CGSize)size
{
    if (_currentAsset == nil) {
        return CGSizeZero;
    }

    CGFloat maxWidth = size.width;
    CGFloat maxHeight = size.height;

    if (maxWidth == CGFLOAT_MAX) {
        maxWidth = -1;
    }
    if (maxHeight == CGFLOAT_MAX) {
        maxHeight = -1;
    }

    CGFloat measuredWidth = (CGFloat)[_currentAsset measureWidth:(double)maxWidth maxHeight:(double)maxHeight];
    CGFloat measuredHeight = (CGFloat)[_currentAsset measureHeight:(double)maxWidth maxHeight:(double)maxHeight];

    return CGSizeMake(measuredWidth, measuredHeight);
}

#pragma mark - UIView

- (CGSize)intrinsicContentSize
{
    CGSize maxSize = CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX);
    return [self sizeThatFits:maxSize];
}

#pragma mark - UIAccessibilityElement

- (BOOL)isAccessibilityElement
{
    return YES;
}

- (NSString *)accessibilityLabel
{
    return [_imageView accessibilityLabel];
}

- (NSString *)accessibilityHint
{
    return [_imageView accessibilityHint];
}

- (NSString *)accessibilityValue
{
    return [_imageView accessibilityValue];
}

- (UIAccessibilityTraits)accessibilityTraits
{
    return [_imageView accessibilityTraits];
}

- (BOOL)requiresLayoutWhenAnimatingBounds
{
    return YES;
}

@end
