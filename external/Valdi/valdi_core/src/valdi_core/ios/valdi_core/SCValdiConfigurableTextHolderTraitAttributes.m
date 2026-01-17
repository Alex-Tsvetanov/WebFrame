
#import "valdi_core/SCValdiConfigurableTextHolderTraitAttributes.h"

BOOL SCValdiConfigurableTextHolderSetTextAlignment(UIView<SCValdiConfigurableTextHolder> *view, NSString *textAlign, BOOL isRightToLeft)
{
    textAlign = [textAlign lowercaseString];
    if ([textAlign isEqualToString:@"right"]) {
        if (isRightToLeft) {
            view.textAlignment = NSTextAlignmentLeft;
        } else {
            view.textAlignment = NSTextAlignmentRight;
        }
        return YES;
    }
    if ([textAlign isEqualToString:@"center"]) {
        view.textAlignment = NSTextAlignmentCenter;
        return YES;
    }
    if (textAlign.length == 0 || [textAlign isEqualToString:@"left"]) {
        view.textAlignment = NSTextAlignmentNatural;
        return YES;
    }
    return NO;
}
