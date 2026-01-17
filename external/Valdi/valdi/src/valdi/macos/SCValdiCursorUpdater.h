//
//  SCValdiCursorUpdater.h
//  valdi-macos
//
//  Created by Simon Corsin on 10/29/21.
//

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class SCValdiCursorUpdater;

typedef NS_ENUM(NSUInteger, SCValdiCursorType) {
    SCValdiCursorTypePointer,
    SCValdiCursorTypeHand,
    SCValdiCursorTypeOpenHand,
};

@protocol SCValdiCursorUpdaterDelegate <NSObject>

- (SCValdiCursorType)cursorUpdater:(SCValdiCursorUpdater*)cursorUpdater cursorTypeAtPoint:(CGPoint)point;

@end

@interface SCValdiCursorUpdater : NSResponder

@property (weak, nonatomic) id<SCValdiCursorUpdaterDelegate> delegate;
@property (assign, nonatomic) CGSize trackingAreaSize;

- (instancetype)initWithView:(NSView*)view;

@end

NS_ASSUME_NONNULL_END
