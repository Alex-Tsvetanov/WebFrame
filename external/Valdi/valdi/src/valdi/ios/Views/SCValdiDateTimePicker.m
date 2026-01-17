#import "valdi/ios/Views/SCValdiDateTimePicker.h"
#import "valdi/ios/Views/SCValdiDatePickerUtils.h"

#import "valdi/ios/Categories/UIView+Valdi.h"

@implementation SCValdiDateTimePicker {
    id<SCValdiFunction> _Nullable _onChange;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.datePickerMode = UIDatePickerModeDateAndTime;
        SCValdiFixIOS14DatePicker(self);
        [self addTarget:self action:@selector(_handleOnChange) forControlEvents:UIControlEventValueChanged];
    }
    return self;
}

- (CGPoint)convertPoint:(CGPoint)point fromView:(UIView *)view
{
    return [self valdi_convertPoint:point fromView:view];
}

- (CGPoint)convertPoint:(CGPoint)point toView:(UIView *)view
{
    return [self valdi_convertPoint:point toView:view];
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    if (self.valdiHitTest != nil) {
        return [self valdi_hitTest:point withEvent:event withCustomHitTest:self.valdiHitTest];
    }
    return [super hitTest:point withEvent:event];
}

#pragma mark - UIView+Valdi

- (BOOL)willEnqueueIntoValdiPool {
    return NO;
}

#pragma mark - Action handling methods

INTERNED_STRING_CONST("dateSeconds", SCValdiDateTimePickerDateInSecondsKey);

#pragma mark - Internal methods


- (void)valdi_setOnChange:(id<SCValdiFunction>)onChange
{
    _onChange = onChange;
}

- (void)_handleOnChange
{
    NSTimeInterval dateSeconds = self.date.timeIntervalSince1970;
    [self.valdiContext didChangeValue:@(dateSeconds)
            forInternedValdiAttribute:SCValdiDateTimePickerDateInSecondsKey()
                              inViewNode:self.valdiViewNode];
    
    if (!_onChange) {
        return;
    }
    
    // pseudocode: _onChange.call({ "dateSeconds": dateSeconds })
    SCValdiMarshallerScoped(marshaller, {
        NSInteger objectIndex = SCValdiMarshallerPushMap(marshaller, 1);
        SCValdiMarshallerPushDouble(marshaller, dateSeconds);
        SCValdiMarshallerPutMapProperty(marshaller, SCValdiDateTimePickerDateInSecondsKey(), objectIndex);
        
        [_onChange performWithMarshaller:marshaller];
    });
}

#pragma mark - Static methods

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder {
    [attributesBinder bindAttribute:@"dateSeconds"
           invalidateLayoutOnChange:NO
                    withDoubleBlock:^BOOL(SCValdiDateTimePicker *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        NSDate *date = [NSDate dateWithTimeIntervalSince1970:attributeValue];
        BOOL animated = animator != nil;
        [view setDate:date animated:animated];
        return YES;
    } resetBlock:^(SCValdiDateTimePicker *view, id<SCValdiAnimatorProtocol> animator) {
        BOOL animated = animator != nil;
        [view setDate:[NSDate date] animated:animated];
    }];

    [attributesBinder bindAttribute:@"minimumDateSeconds"
           invalidateLayoutOnChange:NO
                    withDoubleBlock:^BOOL(SCValdiDateTimePicker *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        NSDate *minDate = [NSDate dateWithTimeIntervalSince1970:attributeValue];
        view.minimumDate = minDate;
        return YES;
    } resetBlock:^(SCValdiDateTimePicker *view, id<SCValdiAnimatorProtocol> animator) {
        view.minimumDate = nil;
    }];

    [attributesBinder bindAttribute:@"maximumDateSeconds"
           invalidateLayoutOnChange:NO
                    withDoubleBlock:^BOOL(SCValdiDateTimePicker *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
        NSDate *maxDate = [NSDate dateWithTimeIntervalSince1970:attributeValue];
        view.maximumDate = maxDate;
        return YES;
    } resetBlock:^(SCValdiDateTimePicker *view, id<SCValdiAnimatorProtocol> animator) {
        view.maximumDate = nil;
    }];

    [attributesBinder bindAttribute:@"onChange"
                  withFunctionBlock:^(SCValdiDateTimePicker *view, id<SCValdiFunction> attributeValue) {
        [view valdi_setOnChange:attributeValue];
    }
                         resetBlock:^(SCValdiDateTimePicker *view) {
        [view valdi_setOnChange:nil];
    }];


    [attributesBinder setPlaceholderViewMeasureDelegate:^UIView *{
        return [SCValdiDateTimePicker new];
    }];
}

@end
