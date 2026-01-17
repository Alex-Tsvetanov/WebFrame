//
//  SCValdiActionHandlerHolder.h
//  Valdi
//
//  Created by Simon Corsin on 5/3/18.
//

#import <Foundation/Foundation.h>

/**
 Object which just maintains a reference to an action handler provider,
 which can provide a handler for a particular action.
 */
@interface SCValdiActionHandlerHolder : NSObject

@property (nonatomic, weak) id actionHandler;

@end
