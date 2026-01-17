//
//  SCCacheDirectoryUtils.m
//  valdi-desktop-apple
//
//  Created by Ramzy Jaber on 4/6/22.
//

#import "SCDirectoryUtils.h"
#import <Foundation/Foundation.h>
#import "SCValdiObjCUtils.h"

namespace ValdiMacOS {

static Valdi::StringBox resolveDirectory(NSString *directorySuffix, bool relativeToTemporaryDirectory) {
    NSString *baseDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    if (!baseDir || relativeToTemporaryDirectory) {
        baseDir = NSTemporaryDirectory();
    }

    NSURL *directoryURL = [[[NSURL fileURLWithPath:baseDir] URLByAppendingPathComponent:@".valdi/desktop"] URLByAppendingPathComponent:directorySuffix];
    return StringFromNSString(directoryURL.path);
}

Valdi::StringBox resolveDocumentsDirectory(bool relativeToTemporaryDirectory) {
    return resolveDirectory(@"documents", relativeToTemporaryDirectory);
}

Valdi::StringBox resolveCachesDirectory(bool relativeToTemporaryDirectory) {
    return resolveDirectory(@"caches", relativeToTemporaryDirectory);
}

Valdi::StringBox resolveShaderCacheDirectory(bool relativeToTemporaryDirectory){
    return resolveDirectory(@"shaders", relativeToTemporaryDirectory);
}

}
