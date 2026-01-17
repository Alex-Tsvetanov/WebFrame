#import <Foundation/Foundation.h>

/**
 * Define possible scroll directions to check for scroll capabilities
 */
typedef NS_ENUM(NSUInteger, SCValdiScrollDirection) {
    SCValdiScrollDirectionTopToBottom = 0,
    SCValdiScrollDirectionBottomToTop = 1,
    SCValdiScrollDirectionLeftToRight = 2,
    SCValdiScrollDirectionRightToLeft = 3,
    SCValdiScrollDirectionHorizontalForward = 4,
    SCValdiScrollDirectionHorizontalBackward = 5,
    SCValdiScrollDirectionVerticalForward = 6,
    SCValdiScrollDirectionVerticalBackward = 7,
    SCValdiScrollDirectionForward = 8,
    SCValdiScrollDirectionBackward = 9,
};
