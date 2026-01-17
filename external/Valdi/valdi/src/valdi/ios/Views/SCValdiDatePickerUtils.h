#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// UIDatePicker defaults to a new "compact" date picker style starting from iOS 14,
/// this function configures a UIDatePicker instance to use the old style.
void SCValdiFixIOS14DatePicker(UIDatePicker* datePicker);

NS_ASSUME_NONNULL_END
