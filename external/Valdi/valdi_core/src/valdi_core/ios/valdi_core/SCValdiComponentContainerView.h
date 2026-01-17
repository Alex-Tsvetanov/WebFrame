//
//  SCValdiComponentContainerView.h
//  valdi-ios
//
//  Created by Simon Corsin on 5/20/19.
//

#import "valdi_core/SCValdiRootView.h"

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiComponentContainerView : SCValdiRootView

- (instancetype)initWithComponentPath:(NSString*)componentPath
                                owner:(nullable id<SCValdiViewOwner>)owner
                              runtime:(nonnull id<SCValdiRuntimeProtocol>)runtime;

- (instancetype)initWithComponentPath:(NSString*)componentPath
                                owner:(nullable id<SCValdiViewOwner>)owner
                            viewModel:(nullable id)viewModel
                     componentContext:(nullable id)componentContext
                              runtime:(nonnull id<SCValdiRuntimeProtocol>)runtime;

@end

NS_ASSUME_NONNULL_END
