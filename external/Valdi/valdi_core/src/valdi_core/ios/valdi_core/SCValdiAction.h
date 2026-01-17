//
//  SCValdiAction.h
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import <Foundation/Foundation.h>

extern NSString* const SCValdiActionSenderKey;

@protocol SCValdiAction <NSObject>

- (void)performWithSender:(id)sender;

// This can be called from any thread, so this needs to be thread safe.
- (id)performWithParameters:(NSArray<id>*)parameters;

@end
