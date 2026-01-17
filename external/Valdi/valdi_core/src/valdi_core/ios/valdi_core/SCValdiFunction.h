//
//  SCValdiFunction.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/31/19.
//

#import "valdi_core/SCValdiMarshaller.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiFunction <NSObject>

/**
 Call the function, using the marshaller as the arguments.
 Return whether a value was pushed onto the marshaller.
 */
- (BOOL)performWithMarshaller:(SCValdiMarshallerRef)marshaller;

/**
 Call the function, using the marshaller as the arguments.
 Return whether a value was pushed onto the marshaller.
 If the protocol implements this sync variant, the function
 should be guaranteed to be called synchronously. If propagatesError is true,
 any error raised within that call will be re-thrown as an Objective-C exception.
 */
@optional
- (BOOL)performSyncWithMarshaller:(SCValdiMarshallerRef)marshaller propagatesError:(BOOL)propagatesError;

/**
 Call the function, using the marshaller as the arguments.
 Returns whether a value was pushed onto the marshaller.
 If the protocol implements this throttle variant, the function
 call will be throttled if a next call using performThrottledWithMarshaller:
 is made before this call actually started executing. This is relevant
 for asynchronous function calls only.
 */
@optional
- (BOOL)performThrottledWithMarshaller:(SCValdiMarshallerRef)marshaller;

@end

void SCValdiFunctionSafePerform(id<SCValdiFunction> function, SCValdiMarshallerRef marshaller);
void SCValdiFunctionSafePerformSync(id<SCValdiFunction> function,
                                    SCValdiMarshallerRef marshaller,
                                    BOOL propagatesError);
void SCValdiFunctionSafePerformThrottled(id<SCValdiFunction> function, SCValdiMarshallerRef marshaller);

NS_ASSUME_NONNULL_END
