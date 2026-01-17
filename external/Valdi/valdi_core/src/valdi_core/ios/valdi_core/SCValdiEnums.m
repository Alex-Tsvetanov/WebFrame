//
//  SCValdiEnums.m
//  valdi-ios
//
//  Created by Simon Corsin on 6/8/20.
//

#import "valdi_core/SCValdiEnums.h"

@implementation SCValdiIntEnum {
    NSInteger *_enumCases;
    NSInteger _enumCasesCount;
}

- (instancetype)initWithEnumCases:(NSInteger *)enumCases count:(NSUInteger)count
{
    self = [super init];

    if (self) {
        _enumCases = enumCases;
        _enumCasesCount = (NSInteger)count;
    }

    return self;
}

- (instancetype)initWithEnumCasesCount:(NSUInteger)enumCasesCount
{
    self = [super init];

    if (self) {
        _enumCases = nil;
        _enumCasesCount = enumCasesCount;
    }

    return self;
}

- (NSInteger)enumCaseForIndex:(NSUInteger)index
{
    if (_enumCases != nil) {
        return _enumCases[index];
    } else {
        return index;
    }
}

- (NSUInteger)count
{
    return _enumCasesCount;
}

@end

@implementation SCValdiStringEnum {
    NSArray<NSString *> *_enumCases;
}

- (instancetype)initWithEnumCases:(NSArray<NSString *> *)enumCases
{
    self = [super init];

    if (self) {
        _enumCases = enumCases;
    }

    return self;
}

- (NSString *)enumCaseForIndex:(NSUInteger)index
{
    return _enumCases[index];
}

- (NSUInteger)count
{
    return _enumCases.count;
}

@end
