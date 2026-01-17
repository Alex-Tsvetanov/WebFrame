//
//  SCValdiJSQueueDispatcher.h
//  Valdi
//
//  Created by Simon Corsin on 9/6/18.
//

#import <Foundation/Foundation.h>

@protocol SCValdiJSQueueDispatcher <NSObject>

- (void)dispatchOnJSQueueWithBlock:(dispatch_block_t)block sync:(BOOL)sync;

@end
