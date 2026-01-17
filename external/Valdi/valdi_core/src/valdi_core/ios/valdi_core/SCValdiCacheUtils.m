//
//  SCValdiCacheUtils.m
//  valdi-ios
//
//  Created by saniul on 24/04/2019.
//

#import "valdi_core/SCValdiCacheUtils.h"

const NSNotificationName SCValdiCacheDirectoryChangedNotificationKey = @"SCValdiCacheDirectoryChanged";

static NSURL *_cacheDirectoryURL;

NSURL *SCValdiGetCacheDirectoryURL() {
    NSURL *url = _cacheDirectoryURL;
    if (!url) {
        NSString *baseDir = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject;
        url = [NSURL fileURLWithPath:baseDir];
    }

    return url;
}

void SCValdiSetCacheDirectoryURL(NSURL *directory) {
    _cacheDirectoryURL = directory;
    [[NSNotificationCenter defaultCenter] postNotificationName:SCValdiCacheDirectoryChangedNotificationKey object:nil];
}
