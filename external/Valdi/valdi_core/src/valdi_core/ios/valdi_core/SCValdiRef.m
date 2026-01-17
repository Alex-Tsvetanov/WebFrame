//
//  SCCValdiRef.m
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/18.
//

#import "valdi_core/SCValdiRef.h"
#import "valdi_core/SCValdiMarshallableObject.h"

@interface SCValdiRef () <SCValdiMarshallableObject>
@end

@implementation SCValdiRef {
    __weak id _weakRef;
    id _strongRef;
    BOOL _isStrong;
}

- (instancetype)initWithInstance:(id)instance strong:(BOOL)strong
{
    self = [super init];

    if (self) {
        _isStrong = strong;
        self.instance = instance;
    }

    return self;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _isStrong = YES;
    }

    return self;
}

- (void)setInstance:(id)instance
{
    if (_isStrong) {
        _strongRef = instance;
        _weakRef = nil;
    } else {
        _strongRef = nil;
        _weakRef = instance;
    }
}

- (id)instance
{
    if (_isStrong) {
        return _strongRef;
    } else {
        return _weakRef;
    }
}

- (void)makeStrong
{
    if (!_isStrong) {
        id instance = self.instance;
        _isStrong = YES;
        self.instance = instance;
    }
}

- (void)makeWeak
{
    if (_isStrong) {
        id instance = self.instance;
        _isStrong = NO;
        self.instance = instance;
    }
}

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    return SCValdiMarshallableObjectDescriptorMake(nil, nil, nil, SCValdiMarshallableObjectTypeUntyped);
}

@end
