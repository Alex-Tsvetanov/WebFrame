//
//  SCValdiVideoPlayer.h
//  valdi_core
//
//  Created by Carson Holgate on 3/17/23.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIView.h>

#import "valdi_core/SCValdiFunction.h"

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiVideoPlayer <NSObject>

@property (readonly, nonatomic) CGSize size;

- (UIView*)getView;
- (void)setVolume:(float)volume;
- (void)setPlaybackRate:(float)playbackRate;
- (void)setSeekToTime:(float)seekToTime;
- (void)setOnVideoLoadedCallback:(id<SCValdiFunction>)callback;
- (void)setOnBeginPlayingCallback:(id<SCValdiFunction>)callback;
- (void)setOnErrorCallback:(id<SCValdiFunction>)callback;
- (void)setOnCompletedCallback:(id<SCValdiFunction>)callback;
- (void)setOnProgressUpdatedCallback:(id<SCValdiFunction>)callback;

@end

NS_ASSUME_NONNULL_END
