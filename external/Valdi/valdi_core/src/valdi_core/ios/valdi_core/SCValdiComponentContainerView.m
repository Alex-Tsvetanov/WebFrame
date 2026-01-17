//
//  SCValdiComponentContainerView.m
//  valdi-ios
//
//  Created by Simon Corsin on 5/20/19.
//

#import "valdi_core/SCValdiComponentContainerView.h"

@implementation SCValdiComponentContainerView {
    NSString *_componentPath;
}

- (instancetype)initWithComponentPath:(NSString *)componentPath owner:(id<SCValdiViewOwner>)owner runtime:(id<SCValdiRuntimeProtocol>)runtime
{
    return [self initWithComponentPath:componentPath owner:owner viewModel:nil componentContext:nil runtime:runtime];
}

- (instancetype)initWithComponentPath:(NSString *)componentPath owner:(id<SCValdiViewOwner>)owner viewModel:(id)viewModel componentContext:(id)componentContext runtime:(id<SCValdiRuntimeProtocol>)runtime
{
    _componentPath = componentPath;
    return [super initWithOwner:owner viewModel:viewModel componentContext:componentContext runtime:runtime];
}

- (NSString *)componentPath
{
    return _componentPath;
}

@end
