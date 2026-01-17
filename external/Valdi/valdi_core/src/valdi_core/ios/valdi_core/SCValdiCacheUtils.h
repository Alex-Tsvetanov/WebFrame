//
//  SCValdiCacheUtils.h
//  valdi-ios
//
//  Created by saniul on 24/04/2019.
//

#import "valdi_core/SCMacros.h"
#import <Foundation/Foundation.h>

SC_EXTERN_C_BEGIN

extern const NSNotificationName SCValdiCacheDirectoryChangedNotificationKey;

extern NSURL* SCValdiGetCacheDirectoryURL();
extern void SCValdiSetCacheDirectoryURL(NSURL* directory);

SC_EXTERN_C_END
