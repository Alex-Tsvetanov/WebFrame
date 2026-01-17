//
//  SCValdiInternedString.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/20/19.
//

#import "valdi_core/SCValdiInternedStringBase.h"
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

NS_ASSUME_NONNULL_BEGIN

SCValdiInternedStringRef SCValdiInternedStringCreate(const char* cLiteral);
void SCValdiInternedStringDestroy(SCValdiInternedStringRef internedString);

SCValdiInternedStringRef SCValdiInternedStringFromNSString(NSString* nsString);
const char* SCValdiInternedStringGetCStr(SCValdiInternedStringRef internedString);

NS_ASSUME_NONNULL_END

#define INTERNED_STRING_CONST(key, functionName)                                                                       \
    static SCValdiInternedString* functionName() {                                                                     \
        static SCValdiInternedStringRef internedString;                                                                \
        static dispatch_once_t onceToken;                                                                              \
        dispatch_once(&onceToken, ^{                                                                                   \
            internedString = SCValdiInternedStringCreate(key);                                                         \
        });                                                                                                            \
        return internedString;                                                                                         \
    }

#ifdef __cplusplus
}
#endif
