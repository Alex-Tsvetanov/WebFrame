//
//  SCValdiKeychainStore.m
//  valdi-ios
//
//  Created by Simon Corsin on 4/9/20.
//

#import "valdi/ios/SCValdiKeychainStore.h"
#import "valdi_core/SCValdiResult.h"

@implementation SCValdiKeychainStore

- (NSMutableDictionary *)_makeSecQueryWithKey:(NSString *)key
{
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    [dict setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
    [dict setObject:@"VALDI" forKey:(__bridge id)kSecAttrService];
    [dict setObject:key forKey:(__bridge id)kSecAttrAccount];
    return dict;
}

- (SCValdiResult *)store:(nonnull NSString *)key
        value:(nonnull NSData *)value
{
    NSMutableDictionary *query = [self _makeSecQueryWithKey:key];
    [query setObject:value forKey:(__bridge id)kSecValueData];
    [query setObject:(__bridge id)kSecAttrAccessibleWhenUnlockedThisDeviceOnly
              forKey:(__bridge id)kSecAttrAccessible];

    OSStatus res = SecItemAdd((__bridge CFDictionaryRef)query, nil);
    if (res == errSecDuplicateItem) {
        // Key already exist, update it...
        NSDictionary *dictUpdate = @{(__bridge id) kSecValueData : value};
        res = SecItemUpdate((__bridge CFDictionaryRef)query, (__bridge CFDictionaryRef)dictUpdate);
    }

    SCValdiResult *result;
    if (res == errSecSuccess) {
        result = SCValdiResultSuccess();
    } else {
        if (@available(iOS 11.3, *)) {
            CFStringRef errorMessage = SecCopyErrorMessageString(res, NULL);
            result = SCValdiResultFailure((__bridge NSString *)(errorMessage));
            CFRelease(errorMessage);
        } else {
            result = SCValdiResultFailure([NSString stringWithFormat:@"Sec error code: %d", (int)res]);
        }
    }

    return result;
}

- (nonnull NSData *)get:(nonnull NSString *)key
{
    NSMutableDictionary *query = [self _makeSecQueryWithKey:key];
    [query setObject:(id)kCFBooleanTrue forKey:(__bridge id)kSecReturnData];

    CFTypeRef cfdata = NULL;
    OSStatus res = SecItemCopyMatching((__bridge CFDictionaryRef)query, &cfdata);
    NSData *data = (id)CFBridgingRelease(cfdata);
    if (res != errSecSuccess || !data) {
        return [NSData data];
    }

    return data;
}

- (BOOL)erase:(nonnull NSString *)key
{
    NSMutableDictionary *query = [self _makeSecQueryWithKey:key];
    return SecItemDelete((__bridge CFDictionaryRef)query) == errSecSuccess;
}

@end
