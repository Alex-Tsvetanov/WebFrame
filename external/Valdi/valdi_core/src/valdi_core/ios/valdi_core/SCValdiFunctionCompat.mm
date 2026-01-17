//
//  SCValdiFunctionCompat.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/23/19.
//

#import "valdi_core/SCValdiFunctionCompat.h"

#import "valdi_core/SCValdiAction.h"
#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/SCValdiMarshaller+CPP.h"

@implementation SCValdiFunctionCompat

- (BOOL)performWithMarshaller:(nonnull SCValdiMarshallerRef)marshaller {
    if (![self respondsToSelector:@selector(performWithParameters:)]) {
        return NO;
    }

    NSArray<id> *parameters = SCValdiMarshallerToUntypedArray(marshaller);
    id value = [((id<SCValdiAction>)self) performWithParameters:parameters];
    if (!value) {
        return NO;
    }
    auto cppValue = ValdiIOS::ValueFromNSObject(value);
    SCValdiMarshallerUnwrap(marshaller)->push(std::move(cppValue));
    return YES;
}

@end
