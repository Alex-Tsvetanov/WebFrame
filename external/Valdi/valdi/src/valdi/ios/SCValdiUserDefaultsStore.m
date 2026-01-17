//
//  SCValdiUserDefaultsStore.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/7/20.
//

#import "SCValdiUserDefaultsStore.h"

@implementation SCValdiUserDefaultsStore

static NSString *SCValdiUserDefaultsStoreMakeKey(NSString *key) {
    return [@"valdi.keychain." stringByAppendingString:key];
}

- (BOOL)erase:(nonnull NSString *)key
{
    NSString *resolvedKey = SCValdiUserDefaultsStoreMakeKey(key);
    if (![[NSUserDefaults standardUserDefaults] dataForKey:resolvedKey]) {
        return NO;
    }

    [[NSUserDefaults standardUserDefaults] removeObjectForKey:resolvedKey];

    return YES;
}

- (nonnull NSData *)get:(nonnull NSString *)key
{
    NSString *resolvedKey = SCValdiUserDefaultsStoreMakeKey(key);
    NSData *data = [[NSUserDefaults standardUserDefaults] dataForKey:resolvedKey];
    return data ?: [NSData data];
}

- (nonnull SCValdiResult *)store:(nonnull NSString *)key value:(nonnull NSData *)value
{
    NSString *resolvedKey = SCValdiUserDefaultsStoreMakeKey(key);
    [[NSUserDefaults standardUserDefaults] setObject:value forKey:resolvedKey];

    return SCValdiResultSuccess();
}

@end
