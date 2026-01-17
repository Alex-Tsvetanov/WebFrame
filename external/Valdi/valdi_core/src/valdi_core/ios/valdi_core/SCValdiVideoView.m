//
//  SCValdiVideoView.m
//  valdi_core
//
//  Created by Carson Holgate on 3/21/23.
//

#import "valdi_core/SCValdiVideoView.h"

#import "valdi_core/NSURL+Valdi.h"
#import "valdi_core/SCValdiActions.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/SCValdiJSConversionUtils.h"
#import "valdi_core/SCValdiImage.h"
#import "valdi_core/SCValdiVideoPlayer.h"
#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCNValdiCoreAssetLoadObserver.h"
#import "valdi_core/UIColor+Valdi.h"
#import "valdi_core/UIImage+Valdi.h"
#import "valdi_core/UIView+ValdiObjects.h"

#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiAttributesBinderBase.h"
#import "valdi_core/SCNValdiCoreCompositeAttributePart.h"
#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiRuntimeProtocol.h"

#import "valdi_core/SCValdiAction.h"
#import <AVFoundation/AVPlayer.h>

@interface SCValdiVideoView() <SCNValdiCoreAssetLoadObserver>

@end

@implementation SCValdiVideoView {
    SCNValdiCoreAsset *_currentAsset;
    id<SCValdiVideoPlayer> _valdiVideoPlayer;
    BOOL _observingAsset;
    UIView *_playerView;
    AVPlayer *_player;
    CGFloat _contentScaleX;
    CGFloat _contentScaleY;
    id<SCValdiFunction> _onVideoLoadedCallback;
    id<SCValdiFunction> _onBeginPlayingCallback;
    id<SCValdiFunction> _onErrorCallback;
    id<SCValdiFunction> _onCompletedCallback;
    id<SCValdiFunction> _onProgressUpdatedCallback;
    double _volume;
    double _playbackRate;
    double _seekToTime;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];

    if (self) {
        _playerView = [[UIView alloc] initWithFrame:frame];
        _contentScaleX = 1;
        _contentScaleY = 1;
        _volume = 1;
        _playbackRate = 0;
        _seekToTime = 0;
        self.clipsToBounds = YES;
    }

    return self;
}

- (void)onValdiAssetDidChange:(nullable id)asset shouldFlip:(BOOL)shouldFlip
{
    [self setValdiVideoPlayer:ProtocolAs(asset, SCValdiVideoPlayer)];
}

- (void)onLoad:(SCNValdiCoreAsset *)asset loadedAsset:(NSObject *)loadedAsset error:(NSString *)error
{
    if (asset != _currentAsset) {
        return;
    }

    [self onValdiAssetDidChange:loadedAsset shouldFlip:NO];
}

- (void)setValdiVideoPlayer:(id<SCValdiVideoPlayer>)valdiVideoPlayer
{
    if (_valdiVideoPlayer != valdiVideoPlayer) {
        _valdiVideoPlayer = valdiVideoPlayer;
        [self _updateVideo];
    }
}

- (void)_updateVideo
{
    // Remove the existing playerView and add the new one as a subView.
    [_playerView removeFromSuperview];

    _playerView = [_valdiVideoPlayer getView];
    _playerView.frame = self.bounds;

    [self addSubview:_playerView];

    // Reset attributes
    [_valdiVideoPlayer setVolume:_volume];
    [_valdiVideoPlayer setPlaybackRate:_playbackRate];
    [_valdiVideoPlayer setSeekToTime:_seekToTime];
    [_valdiVideoPlayer setOnVideoLoadedCallback:_onVideoLoadedCallback];
    [_valdiVideoPlayer setOnBeginPlayingCallback:_onBeginPlayingCallback];
    [_valdiVideoPlayer setOnErrorCallback:_onErrorCallback];
    [_valdiVideoPlayer setOnCompletedCallback:_onCompletedCallback];
    [_valdiVideoPlayer setOnProgressUpdatedCallback:_onProgressUpdatedCallback];

    [_valdiVideoPlayer setSeekToTime:0];
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
            [_currentAsset addLoadObserver:self outputType:SCNValdiCoreAssetOutputTypeVideoIOS preferredWidth:0 preferredHeight:0 filter:[NSNull null]];
        }
    }
}

- (BOOL)valdi_setObjectFit:(NSString *)attributeValue
{
    if (!attributeValue) {
        _playerView.contentMode = UIViewContentModeScaleToFill;
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

    _playerView.contentMode = contentMode;

    return YES;
}

- (void)setAsset:(SCNValdiCoreAsset *)asset
       tintColor:(UIColor *)tintColor
       flipOnRtl:(BOOL)flipOnRtl
{
    BOOL shouldFlip = flipOnRtl ? self.effectiveUserInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft : NO;
    [self _applyAsset:asset shouldFlip:shouldFlip];
}

- (BOOL)valdi_setContentTransform:(id)attributeValue animator:(id<SCValdiAnimatorProtocol>)animator
{
    if (attributeValue != nil) {
        NSArray *parts = ObjectAs(attributeValue, NSArray);
        if (parts.count != 2) {
            return NO;
        }
        _contentScaleX = (ObjectAs(parts[0], NSNumber) ?: @(1.0)).doubleValue;
        _contentScaleY = (ObjectAs(parts[1], NSNumber) ?: @(1.0)).doubleValue;
    } else {
        _contentScaleX = 1;
        _contentScaleY = 1;
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
    [attributesBinder bindAssetAttributesForOutputType:SCNValdiCoreAssetOutputTypeVideoIOS];

    [attributesBinder bindAttribute:@"onVideoLoaded"
            withFunctionBlock:^(SCValdiVideoView *view, id<SCValdiFunction> attributeValue) {
              [view valdi_setOnVideoLoadedCallback:attributeValue];
            }
            resetBlock:^(__kindof SCValdiVideoView *view) {
              [view valdi_setOnVideoLoadedCallback:nil];
            }];


    [attributesBinder bindAttribute:@"onBeginPlaying"
              withFunctionBlock:^(SCValdiVideoView *view, id<SCValdiFunction> attributeValue) {
                [view valdi_setOnBeginPlayingCallback:attributeValue];
              }
              resetBlock:^(__kindof SCValdiVideoView *view) {
                [view valdi_setOnBeginPlayingCallback:nil];
              }];

    [attributesBinder bindAttribute:@"onError"
            withFunctionBlock:^(SCValdiVideoView *view, id<SCValdiFunction> attributeValue) {
              [view valdi_setOnErrorCallback:attributeValue];
            }
            resetBlock:^(__kindof SCValdiVideoView *view) {
              [view valdi_setOnErrorCallback:nil];
            }];

    [attributesBinder bindAttribute:@"onCompleted"
            withFunctionBlock:^(SCValdiVideoView *view, id<SCValdiFunction> attributeValue) {
              [view valdi_setOnCompletedCallback:attributeValue];
            }
            resetBlock:^(__kindof SCValdiVideoView *view) {
              [view valdi_setOnCompletedCallback:nil];
            }];

    [attributesBinder bindAttribute:@"onProgressUpdated"
            withFunctionBlock:^(SCValdiVideoView *view, id<SCValdiFunction> attributeValue) {
              [view valdi_setOnProgressUpdatedCallback:attributeValue];
            }
            resetBlock:^(__kindof SCValdiVideoView *view) {
              [view valdi_setOnProgressUpdatedCallback:nil];
            }];

    [attributesBinder bindAttribute:@"playbackRate"
           invalidateLayoutOnChange:false withDoubleBlock:^BOOL(SCValdiVideoView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        return [view valdi_setPlaybackRate:attributeValue];
    } resetBlock:^(SCValdiVideoView *view, id<SCValdiAnimatorProtocol> animator) {
        // Default to paused;
        [view valdi_setPlaybackRate:0];
    }];

    [attributesBinder bindAttribute:@"volume"
           invalidateLayoutOnChange:false withDoubleBlock:^BOOL(SCValdiVideoView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        return [view valdi_setVolume:attributeValue];
    } resetBlock:^(SCValdiVideoView *view, id<SCValdiAnimatorProtocol> animator) {
        // Default to full volume;
        [view valdi_setVolume:1];
    }];

    [attributesBinder bindAttribute:@"seekToTime"
           invalidateLayoutOnChange:false withDoubleBlock:^BOOL(SCValdiVideoView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        return [view valdi_setSeekToTime:attributeValue];
    } resetBlock:^(SCValdiVideoView *view, id<SCValdiAnimatorProtocol> animator) {
        [view valdi_setSeekToTime:0];
    }];
}

- (void)valdi_setOnBeginPlayingCallback:(id<SCValdiFunction>)callback {
    _onBeginPlayingCallback = callback;
    [_valdiVideoPlayer setOnBeginPlayingCallback:_onBeginPlayingCallback];
}

- (void)valdi_setOnVideoLoadedCallback:(id<SCValdiFunction>)callback {
    _onVideoLoadedCallback = callback;
    [_valdiVideoPlayer setOnVideoLoadedCallback:_onVideoLoadedCallback];
}

- (void)valdi_setOnErrorCallback:(id<SCValdiFunction>)callback {
    _onErrorCallback = callback;
    [_valdiVideoPlayer setOnErrorCallback:_onErrorCallback];
}

- (void)valdi_setOnCompletedCallback:(id<SCValdiFunction>)callback {
    _onCompletedCallback = callback;
    [_valdiVideoPlayer setOnCompletedCallback:_onCompletedCallback];
}

- (void)valdi_setOnProgressUpdatedCallback:(id<SCValdiFunction>)callback {
    _onProgressUpdatedCallback = callback;
    [_valdiVideoPlayer setOnProgressUpdatedCallback:_onProgressUpdatedCallback];
}

- (BOOL)valdi_setVolume:(CGFloat)volume {
    _volume = volume;
    [_valdiVideoPlayer setVolume:_volume];
    return YES;
}

- (BOOL)valdi_setSeekToTime:(CGFloat)seekToTime {
    _seekToTime = seekToTime;
    [_valdiVideoPlayer setSeekToTime:_seekToTime];
    return YES;
}

- (BOOL)valdi_setPlaybackRate:(CGFloat)playbackRate {
    _playbackRate = playbackRate;
    [_valdiVideoPlayer setPlaybackRate:_playbackRate];
    return YES;
}

- (BOOL)willEnqueueIntoValdiPool
{
    return self.class == [SCValdiVideoView class];
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
    _playerView.frame = CGRectMake(-diffWidth / 2, -diffHeight / 2, childWidth, childHeight);
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
    return [_playerView accessibilityLabel];
}

- (NSString *)accessibilityHint
{
    return [_playerView accessibilityHint];
}

- (NSString *)accessibilityValue
{
    return [_playerView accessibilityValue];
}

- (UIAccessibilityTraits)accessibilityTraits
{
    return [_playerView accessibilityTraits];
}

@end

