//
//  SCValdiActionCompat.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/27/19.
//

#import "valdi_core/SCValdiActionCompat.h"
#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiMarshaller.h"

@implementation SCValdiActionCompat

- (void)performWithSender:(id)sender
{
    [self performWithParameters:nil];
}

- (id)performWithParameters:(NSArray<id> *)parameters
{
    if (![self respondsToSelector:@selector(performWithMarshaller:)]) {
        return nil;
    }

    SCValdiMarshallerScoped(marshaller, {
        for (id param in parameters) {
            SCValdiMarshallerPushUntyped(marshaller, param);
        }
        SCValdiFunctionSafePerform((id<SCValdiFunction>)self, marshaller);
        return SCValdiMarshallerGetUntyped(marshaller, -1);
    });
}

@end
