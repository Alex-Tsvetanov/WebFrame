
#import "SCCCircleView.h"
#import "valdi_core/SCValdiAttributesBinderBase.h"

@implementation SCCCircleView {
    CGFloat _thickness;
    UIColor* _color;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];

    if (self) {
        self.opaque = NO;
    }

    return self;
}

- (BOOL)willEnqueueIntoValdiPool {
    return YES;
}

- (void)drawRect:(CGRect)rect {
    if (!_color) {
        return;
    }

    CGFloat halfLine = _thickness * 0.5f;
    CGRect insetRect = CGRectInset(self.bounds, halfLine, halfLine);

    UIBezierPath* circle = [UIBezierPath bezierPathWithOvalInRect:insetRect];
    circle.lineWidth = _thickness;

    [_color setStroke];
    [circle stroke];
}

- (BOOL)valdi_setThickness:(CGFloat)thickness {
    _thickness = thickness;
    [self setNeedsDisplay];
    return YES;
}

- (BOOL)valdi_setColor:(UIColor*)color {
    _color = color;
    [self setNeedsDisplay];
    return YES;
}

+ (CGSize)valdi_onMeasureWithAttributes:(id<SCValdiViewLayoutAttributes>)attributes maxSize:(CGSize)maxSize {
    CGFloat minSize = MIN(maxSize.width, maxSize.height);
    return CGSizeMake(minSize, minSize);
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder {
    [attributesBinder bindAttribute:@"thickness"
        invalidateLayoutOnChange:NO
        withDoubleBlock:^BOOL(__kindof UIView* view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setThickness:attributeValue];
        }
        resetBlock:^(__kindof UIView* view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setThickness:0];
        }];

    [attributesBinder bindAttribute:@"color"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(__kindof UIView* view, UIColor* attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setColor:attributeValue];
        }
        resetBlock:^(__kindof UIView* view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setColor:nil];
        }];

    [attributesBinder setMeasureDelegate:^CGSize(id<SCValdiViewLayoutAttributes> attributes,
                                                 CGSize maxSize,
                                                 UITraitCollection* traitCollection) {
        return [SCCCircleView valdi_onMeasureWithAttributes:attributes maxSize:maxSize];
    }];
}

@end
