//
//  SCValdiError.h
//  valdi-ios
//
//  Created by Simon Corsin on 9/23/19.
//

#import "valdi_core/SCMacros.h"
#import <Foundation/Foundation.h>

SC_EXTERN_C_BEGIN

NS_ASSUME_NONNULL_BEGIN

/**
 Represents an error during marshalling or during a call
 */
@interface SCValdiError : NSException

- (instancetype)initWithReason:(NSString*)reason;
- (instancetype)initWithReason:(NSString*)reason stackTrace:(NSString* _Nullable)stackTrace;

@end

void SCValdiErrorThrow(NSString* reason) __attribute__((noreturn));
void SCValdiErrorThrowWithStacktrace(NSString* reason, NSString* _Nullable stackTrace) __attribute__((noreturn));
void SCValdiErrorRethrow(SCValdiError* initialError, NSString* messagePrefixFormat, ...) __attribute__((noreturn));
void SCValdiErrorThrowUnimplementedMethod() __attribute__((noreturn));

NSString* _Nullable SCValdiErrorGetStackTrace(NSException* exception);

NS_ASSUME_NONNULL_END

SC_EXTERN_C_END
