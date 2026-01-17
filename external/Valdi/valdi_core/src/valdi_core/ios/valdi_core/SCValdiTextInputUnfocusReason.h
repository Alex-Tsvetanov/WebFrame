#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, SCValdiTextInputUnfocusReason) {
    SCValdiTextInputUnfocusReasonUnknown = 0,
    SCValdiTextInputUnfocusReasonReturnKeyPress = 1,
    SCValdiTextInputUnfocusReasonDismissKeyPress = 2
};
