//
//  SCValdiMarshallable.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/25/19.
//

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiMarshaller.h"
#import <Foundation/Foundation.h>

SC_EXTERN_C_BEGIN

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiMarshallable <NSObject>

@optional
/**
 Push the object into the given value stack.
 Returns the index of the object that was pushed
 in the marshaller.
 This method only needs to be implemented if the object is used
 as a viewModel or componentContext object directly.
 */
- (NSInteger)pushToValdiMarshaller:(SCValdiMarshallerRef)marshaller;

@optional
/**
 When the underlying instance should be retained when marshalled into a ValdiMarshaller.
 If not implemented, the instance will be held as a strong reference.
 If returning false, the instance will be held as a weak reference.
 */
- (BOOL)shouldRetainInstanceWhenMarshalling;

@end

NSString* SCValdiMarshallableToString(id<SCValdiMarshallable> marshallable);

/**
 * Compares two ValdiMarshallable instances using a slow method which
 * involves marshalling the items to compare them.
 */
BOOL SCValdiMarshallableSlowEqualsTo(_Nullable id<SCValdiMarshallable> leftMarshallable,
                                     _Nullable id<SCValdiMarshallable> rightMarshallable);

NS_ASSUME_NONNULL_END

SC_EXTERN_C_END
