//
//  SCValdiMarshallable.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/25/19.
//

#import "valdi_core/SCValdiMarshallable.h"

NSString *SCValdiMarshallableToString(id<SCValdiMarshallable> marshallable) {
    SCValdiMarshallerScoped(marshaller, {
        NSInteger objectIndex = [marshallable pushToValdiMarshaller:marshaller];
        return SCValdiMarshallerToString(marshaller, objectIndex, YES);
    });
}

/**
 * Compares two ValdiMarshallable instances using a slow method which
 * involves marshalling the items to compare them.
 */
BOOL SCValdiMarshallableSlowEqualsTo(id<SCValdiMarshallable> leftMarshallable, id<SCValdiMarshallable> rightMarshallable) {
    if (leftMarshallable == rightMarshallable) {
        return YES;
    }
    if (![leftMarshallable respondsToSelector:@selector(pushToValdiMarshaller:)]) {
        return NO;
    }
    if (![rightMarshallable respondsToSelector:@selector(pushToValdiMarshaller:)]) {
        return NO;
    }

    SCValdiMarshallerScoped(leftMarshaller, {
        [leftMarshallable pushToValdiMarshaller:leftMarshaller];

        SCValdiMarshallerScoped(rightMarshaller, {
            [rightMarshallable pushToValdiMarshaller:rightMarshaller];

            return SCValdiMarshallerEquals(leftMarshaller, rightMarshaller);
        });
    });
}
