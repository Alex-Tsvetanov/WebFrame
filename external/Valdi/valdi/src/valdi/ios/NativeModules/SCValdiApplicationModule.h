//
//  SCValdiApplicationModule.h
//  Valdi
//
//  Created by Simon Corsin on 9/19/18.
//

#import "valdi_core/SCNValdiCoreModuleFactory.h"

#import <Foundation/Foundation.h>

@interface SCValdiApplicationModule : NSObject <SCNValdiCoreModuleFactory>

- (void)ensureApplicationModuleIsReadyForContextCreation;

- (void)setIsIntegrationTestEnvironment:(BOOL)isIntegrationTestEnvironment;

@end
