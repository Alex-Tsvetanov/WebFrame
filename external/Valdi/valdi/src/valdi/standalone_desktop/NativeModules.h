//
//  SnapModules.h
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/14/22.
//

#import "valdi/macos/SCValdiRuntime.h"
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

void SCValdiNativeModulesRegister(SCValdiRuntime* runtime);

#ifdef __cplusplus
}
#endif
