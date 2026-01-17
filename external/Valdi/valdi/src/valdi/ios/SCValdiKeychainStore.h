//
//  SCValdiKeychainStore.h
//  valdi-ios
//
//  Created by Simon Corsin on 4/9/20.
//

#import "valdi/SCNValdiKeychain.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiKeychainStore : NSObject <SCNValdiKeychain>

/** Store the given string blob into the keychain */
- (SCValdiResult*)store:(nonnull NSString*)key value:(nonnull NSData*)value;
/**
 * Retrieve the stored blob for the given key.
 * Returns empty string if the value was not found.
 */
- (nonnull NSData*)get:(nonnull NSString*)key;

/** Erase the stored blob for the given key. */
- (BOOL)erase:(nonnull NSString*)key;

@end

NS_ASSUME_NONNULL_END
