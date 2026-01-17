#import "valdi_core/SCValdiIScrollPerfLoggerBridge.h"
#import <Foundation/Foundation.h>

/**
 * Provides a configured instance of the platform-specific
 * attributed scroll view performance logger.
 *
 * @see: SCValdiIScrollPerfLoggerBridge
 */
@protocol SCValdiIScrollPerfLoggerBridgeFactory

- (id<SCValdiIScrollPerfLoggerBridge>)createScrollPerfLoggerBridgeWithTag:(NSString*)tag
                                                                  feature:(NSString*)feature
                                                                  project:(NSString*)project;

@end
