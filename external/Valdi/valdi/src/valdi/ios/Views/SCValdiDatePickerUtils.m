#import <UIKit/UIKit.h>

#ifndef __IPHONE_14_0
#define __IPHONE_14_0 140000
#endif

void SCValdiFixIOS14DatePicker(UIDatePicker *datePicker) {
    // The `client` artifact currently still compiles using Xcode 11.4.1, but
    // it might already be linked into an app built with Xcode 12, which will trigger
    // the new behavior. So, we have to set the preferredDatePickerStyle value even
    // if we don't have access to it in the UIKit headers.
    //
    // That's why we're doing a build-time API availability check in addition to the
    // runtime availability check.
    #if __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_14_0
        if (@available(iOS 13.4, *)) {
            datePicker.preferredDatePickerStyle = UIDatePickerStyleWheels;
        }
    #else
        SEL setPreferredDatePickerStyleSelector = NSSelectorFromString(@"setPreferredDatePickerStyle:");
        if (setPreferredDatePickerStyleSelector && [datePicker respondsToSelector:setPreferredDatePickerStyleSelector]) {
            NSInteger wheelsValue = /*UIDatePickerStyleWheels*/1;

            NSMethodSignature* signature = [[datePicker class] instanceMethodSignatureForSelector:setPreferredDatePickerStyleSelector];
            NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
            [invocation setTarget:datePicker];
            [invocation setSelector:setPreferredDatePickerStyleSelector];
            [invocation setArgument:&wheelsValue atIndex:2];
            [invocation invoke];
        }
    #endif
}
