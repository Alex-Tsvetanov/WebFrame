
#import "SCValdiTextViewEffectsLayoutManager.h"
#import "valdi/ios/Text/NSAttributedString+Valdi.h"

#import <CoreText/CoreText.h>

@implementation SCValdiTextViewBackgroundEffects
@end

@interface SCValdiTextViewOutline : NSObject
@property (nonatomic, assign) NSRange range;
@property (nonatomic, strong) UIColor* color;
@property (nonatomic, assign) CGFloat width;
@end
@implementation SCValdiTextViewOutline
@end


@implementation SCValdiTextViewEffectsLayoutManager

- (UIColor *)backgroundColor
{
    return _effects.color ? _effects.color : [UIColor clearColor];
}

- (CGFloat)backgroundBorderRadius
{
    return _effects.borderRadius ? _effects.borderRadius : 0.0;
}

- (CGFloat)backgroundPadding
{
    return _effects.padding ? _effects.padding : 0.0;
}


#pragma mark - Outline Drawing

/// Drawing a stroke around text (attributed text key `NSStrokeWidthAttributeName`) has two options:
/// 1. A positive value draws a stroke alone around the text glyphs.
/// 2. A negative value draws the text fill and then inner strokes the glphys.
///
/// For a text outline, we want the stoke to draw around each text glyph and then fill the text.
/// This ensures a true outline around each glyph and hides any stroke artifacts that might occur from some fonts.
- (void)drawGlyphsForGlyphRange:(NSRange)glyphsToShow atPoint:(CGPoint)origin
{
    NSRange totalGlyphRange = [self glyphRangeForCharacterRange:NSMakeRange(0, self.textStorage.length) actualCharacterRange:nil];

    NSAttributedString *attributedString = [self.textStorage attributedSubstringFromRange:[self characterRangeForGlyphRange:totalGlyphRange actualGlyphRange:nil]];
    __block NSMutableArray<SCValdiTextViewOutline *> *outlineRanges = [NSMutableArray new];
    [attributedString enumerateAttribute:kSCValdiOuterOutlineWidthAttribute inRange:NSMakeRange(0, attributedString.length) options:0 usingBlock:^(id value, NSRange range, BOOL *stop) {
        if ([value isKindOfClass:[NSNumber class]]) {
            SCValdiTextViewOutline *outline = [SCValdiTextViewOutline new];
            outline.range = range;
            outline.width = [(NSNumber *)value floatValue];
            id colorAttribute = [attributedString attribute:kSCValdiOuterOutlineColorAttribute atIndex:range.location effectiveRange:&range];
            if ([colorAttribute isKindOfClass:[UIColor class]]) {
                outline.color = (UIColor *)colorAttribute;
            }
            [outlineRanges addObject:outline];
        }
    }];
    
    if (outlineRanges.count == 0) {
        // No outlines to draw
        [super drawGlyphsForGlyphRange:glyphsToShow atPoint:origin];
        return;
    }

    // Adjust the origin to account for the size increase of the text container in `usedRectForTextContainer:`
    CGPoint adjustedOrigin = [self _getAdjustedOriginForPoint:origin];
    
    CGContextRef context = UIGraphicsGetCurrentContext();

    // First draw the outlines for the text
    for (SCValdiTextViewOutline *outline in outlineRanges) {
        [self _drawOutline:outline attributedString:attributedString glyphsOrigin:adjustedOrigin context:context];
    }
    
    // Last draw the text itself to cover any strange artifacts that might occur inside the outline's stroke
    [super drawGlyphsForGlyphRange:glyphsToShow atPoint:adjustedOrigin];
}

- (CGRect)usedRectForTextContainer:(NSTextContainer *)container
{
    CGRect rect = [super usedRectForTextContainer:container];
    // Increase the size of the text container to account for the outline's width, otherwise the outline can clip with the edge of the container
    CGFloat maxOutlineWidth = [self _maximumOuterOutlineSize];
    rect.size.width += maxOutlineWidth;
    rect.size.height += maxOutlineWidth;
    return rect;
}

- (CGFloat)_maximumOuterOutlineSize
{
    __block CGFloat maxOutlineWidth = 0.0;
    [self.textStorage enumerateAttribute:kSCValdiOuterOutlineWidthAttribute
                                 inRange:NSMakeRange(0, self.textStorage.length)
                                 options:0
                              usingBlock:^(id value, NSRange range, BOOL *stop) {
        if ([value isKindOfClass:[NSNumber class]]) {
            CGFloat width = [(NSNumber *)value floatValue];
            if (width > maxOutlineWidth) {
                maxOutlineWidth = width;
            }
        }
    }];
    return maxOutlineWidth;
}

- (CGPoint)_getAdjustedOriginForPoint:(CGPoint)origin
{
    CGFloat maxOutlineSize = [self _maximumOuterOutlineSize] / 2.0;
    if (maxOutlineSize <= 0) {
        return origin;
    }
    NSTextAlignment alignment = self.textStorage.length > 0 ?
        [[self.textStorage attribute:NSParagraphStyleAttributeName atIndex:0 effectiveRange:nil] alignment] : NSTextAlignmentLeft;
    CGFloat adjustedX = origin.x;
    switch (alignment) {
        case NSTextAlignmentRight:
            adjustedX -= maxOutlineSize;
            break;
        case NSTextAlignmentCenter:
            adjustedX -= maxOutlineSize / 2.0;
            break;
        case NSTextAlignmentLeft:
        default:
            adjustedX += maxOutlineSize;
            break;
    }
    return CGPointMake(adjustedX, origin.y + maxOutlineSize);
}

- (void)_drawOutline:(SCValdiTextViewOutline *)outline attributedString:(NSAttributedString *)attributedString glyphsOrigin:(CGPoint)origin context:(CGContextRef)context
{
    NSRange charRange = outline.range;
    if (charRange.length == 0) {
        return;
    }

    NSUInteger charIndex = charRange.location;
    NSUInteger charRangeEnd = NSMaxRange(charRange);

    while (charIndex < charRangeEnd) {
        // Get the line fragment range for this character index as the charRange might span multiple lines
        NSRange lineRange;
        [self lineFragmentRectForGlyphAtIndex:[self glyphIndexForCharacterAtIndex:charIndex] effectiveRange:&lineRange];

        // Intersect the outline's range and this line fragment's range
        NSRange intersectionRange = NSIntersectionRange(charRange, lineRange);
        if (intersectionRange.length == 0) {
            charIndex = NSMaxRange(lineRange);
            continue;
        }

        NSRange glyphRange = [self glyphRangeForCharacterRange:intersectionRange actualCharacterRange:nil];
        CGPoint glyphLocation = [self locationForGlyphAtIndex:glyphRange.location];
        CGRect boundingRect = [self boundingRectForGlyphRange:glyphRange inTextContainer:[self textContainerForGlyphAtIndex:glyphRange.location effectiveRange:nil]];
        CGContextSaveGState(context);
        CGContextTranslateCTM(context, origin.x + boundingRect.origin.x, origin.y + glyphLocation.y + boundingRect.origin.y);
        CGContextScaleCTM(context, 1.0, -1.0); // Flip context for CoreText

        // Get each glyph from this line's runs
        // This is done over getting the glyph directly from the layoutmanager as the run accounts for ligatures like "fi"
        NSAttributedString *subAttrString = [attributedString attributedSubstringFromRange:intersectionRange];
        CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)subAttrString);
        CFArrayRef runs = CTLineGetGlyphRuns(line);
        CFIndex runCount = CFArrayGetCount(runs);
        for (CFIndex runIndex = 0; runIndex < runCount; runIndex++) {
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, runIndex);
            CFIndex glyphCount = CTRunGetGlyphCount(run);
            if (glyphCount == 0) {
                continue;
            }
            NSDictionary *runAttrs = (NSDictionary *)CTRunGetAttributes(run);
            CTFontRef runFont = (__bridge CTFontRef)runAttrs[(__bridge id)kCTFontAttributeName];
            if (!runFont) {
                continue;
            }

            CGGlyph glyphs[glyphCount];
            CGPoint positions[glyphCount];
            CTRunGetGlyphs(run, CFRangeMake(0, glyphCount), glyphs);
            CTRunGetPositions(run, CFRangeMake(0, glyphCount), positions);

            for (CFIndex glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++) {
                CGGlyph glyph = glyphs[glyphIndex];
                CGPoint glyphPosition = positions[glyphIndex];
                CGPathRef glyphPath = CTFontCreatePathForGlyph(runFont, glyph, nil);
                CGPathRef strokedGlyphPath = nil;
                if (glyphPath) {
                    // Create a new path instead of stroking the original path as some fonts have overlapping shapes which will cause artifacts to render strangely outside of a true outline
                    strokedGlyphPath = CGPathCreateCopyByStrokingPath(glyphPath, nil, outline.width, kCGLineCapRound, kCGLineJoinRound, 0);
                }
                if (strokedGlyphPath) {
                    CGContextSaveGState(context);
                    CGContextTranslateCTM(context, glyphPosition.x, glyphPosition.y);
                    CGContextAddPath(context, strokedGlyphPath);
                    CGContextSetFillColorWithColor(context, outline.color.CGColor);
                    CGContextFillPath(context);
                    CGContextRestoreGState(context);
                }
                CGPathRelease(glyphPath);
                CGPathRelease(strokedGlyphPath);
            }
        }

        CFRelease(line);
        CGContextRestoreGState(context);

        charIndex = NSMaxRange(intersectionRange);
    }
}


#pragma mark - Background Drawing

- (void)drawBackgroundForGlyphRange:(NSRange)glyphsToShow atPoint:(CGPoint)origin
{
    [super drawBackgroundForGlyphRange:glyphsToShow atPoint:origin];

    if (self.backgroundColor == [UIColor clearColor]) {
        // Don't draw any background if the color is clear
        return;
    }

    // Always render the whole glyph range to ensure there is no invalid cache for backgrounds that are not included in 'glyphsToShow'
    NSRange glyphRange = [self glyphRangeForCharacterRange:NSMakeRange(0, self.textStorage.length) actualCharacterRange:nil];

    NSMutableArray<NSValue *> *lineRects = [NSMutableArray new];
    [self enumerateLineFragmentsForGlyphRange:glyphRange
                                   usingBlock:^(CGRect rect, CGRect usedRect, NSTextContainer *_Nonnull textContainer,
                                                NSRange glyphRange, BOOL *_Nonnull stop) {
                                        NSRange lineFragmentCharacterRange = [self characterRangeForGlyphRange:glyphRange actualGlyphRange:nil];
                                        NSString *lineFragmentString = [self.textStorage.string substringWithRange:lineFragmentCharacterRange];
                                        NSString *trimmedLineFragmentString = [lineFragmentString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
                                        if (trimmedLineFragmentString.length == 0) {
                                            // Don't include empty line fragments
                                            return;
                                        }
                                        NSRange trimmedLineFragmentRange = [lineFragmentString rangeOfString:trimmedLineFragmentString];
                                        NSUInteger whitespaceStartLocation = trimmedLineFragmentRange.location + trimmedLineFragmentRange.length;
                                        if (whitespaceStartLocation < lineFragmentString.length) {
                                            // Remove trailing whitespace from line fragment width
                                            BOOL hasNewLine = [lineFragmentString rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].location != NSNotFound;
                                            NSUInteger whitespaceNewlineOffset = hasNewLine ? 1 : 0;
                                            NSUInteger whitespaceLocation = glyphRange.location + whitespaceStartLocation;
                                            NSRange whitespaceRange = NSMakeRange(whitespaceLocation, glyphRange.location + glyphRange.length - (whitespaceLocation) - whitespaceNewlineOffset);
                                            CGRect whitespaceBoundingRect = [self boundingRectForGlyphRange:whitespaceRange inTextContainer:textContainer];
                                            usedRect.size.width -= whitespaceBoundingRect.size.width;
                                        }

                                       CGRect paddedRect = [self _addVerticalPaddingTo:usedRect
                                                                               padding:self.backgroundPadding / 2.0];
                                       [lineRects addObject:[NSValue valueWithCGRect:paddedRect]];
                                   }];
    [self _processLineRects:lineRects];

    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, origin.x, origin.y);
    [self _drawLineRects:[lineRects copy]];
    CGContextRestoreGState(context);
}

- (CGRect)_addVerticalPaddingTo:(CGRect)rect padding:(CGFloat)padding
{
    return CGRectMake(rect.origin.x, rect.origin.y - padding, rect.size.width, rect.size.height + padding * 2);
}

- (void)_processLineRects:(NSMutableArray<NSValue *> *)lineRects
{
    if (lineRects.count < 2) {
        return;
    }
    NSInteger maxIndex = 0;
    for (NSUInteger i = 1; i < lineRects.count; i++) {
        maxIndex = i;
        [self _processLineRectAtIndex:i maxIndex:maxIndex lineRects:lineRects];
    }
}

- (void)_processLineRectAtIndex:(NSInteger)rectIndex
                       maxIndex:(NSInteger)maxIndex
                      lineRects:(NSMutableArray<NSValue *> *)lineRects
{
    if (lineRects.count < 2 || rectIndex < 1 || rectIndex > maxIndex) {
        return;
    }

    CGRect currentRect = [lineRects objectAtIndex:rectIndex].CGRectValue;
    CGRect previousRect = [lineRects objectAtIndex:rectIndex - 1].CGRectValue;

    BOOL matchPrevious = ((currentRect.origin.x - previousRect.origin.x < 2 * self.backgroundBorderRadius) &&
                          (currentRect.origin.x > previousRect.origin.x)) ||
                         ((CGRectGetMaxX(currentRect) - CGRectGetMaxX(previousRect) > -2 * self.backgroundBorderRadius) &&
                          (CGRectGetMaxX(currentRect) < CGRectGetMaxX(previousRect)));
    BOOL matchCurrent = ((previousRect.origin.x - currentRect.origin.x < 2 * self.backgroundBorderRadius) &&
                         (previousRect.origin.x > currentRect.origin.x)) ||
                        ((CGRectGetMaxX(previousRect) - CGRectGetMaxX(currentRect) > -2 * self.backgroundBorderRadius) &&
                         (CGRectGetMaxX(previousRect) < CGRectGetMaxX(currentRect)));

    if (matchCurrent) {
        // Update the previous rect to match the size of current
        CGRect newPreviousRect =
            CGRectMake(currentRect.origin.x, previousRect.origin.y, currentRect.size.width, previousRect.size.height);
        [lineRects replaceObjectAtIndex:rectIndex - 1 withObject:[NSValue valueWithCGRect:newPreviousRect]];
        // Update rect before if needed
        [self _processLineRectAtIndex:rectIndex - 1 maxIndex:maxIndex lineRects:lineRects];
    } else if (matchPrevious) {
        // Update currect rect to match the size of the previous
        CGRect newCurrentRect =
            CGRectMake(previousRect.origin.x, currentRect.origin.y, previousRect.size.width, currentRect.size.height);
        [lineRects replaceObjectAtIndex:rectIndex withObject:[NSValue valueWithCGRect:newCurrentRect]];
        // Update rect after if needed
        [self _processLineRectAtIndex:rectIndex + 1 maxIndex:maxIndex lineRects:lineRects];
    }
}

/// Must be called from Core Graphics
- (void)_drawLineRects:(NSArray<NSValue *> *)lineRects
{
    UIBezierPath *path = [UIBezierPath new];

    // start by drawing path in the top left down to the bottom left
    for (NSUInteger i = 0; i < lineRects.count; i++) {
        CGRect currentRect = [lineRects objectAtIndex:i].CGRectValue;

        if (i == 0) {
            // start -- get top left to bottom
            [path moveToPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMinY(currentRect) + self.backgroundBorderRadius)];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMinX(currentRect) + self.backgroundBorderRadius, CGRectGetMinY(currentRect))
                       controlPoint:currentRect.origin];
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(currentRect) - self.backgroundBorderRadius, CGRectGetMinY(currentRect))];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMinY(currentRect) + self.backgroundBorderRadius)
                       controlPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMinY(currentRect))];
        }

        NSUInteger nextIndex = i + 1;
        if (nextIndex >= lineRects.count) {
            continue;
        }

        // Draw the right side to the bottom right, and if needed, curve, bottom line, and curve to the next line
        CGRect nextRect = lineRects[nextIndex].CGRectValue;
        CGFloat currentRectMaxX = CGRectGetMaxX(currentRect);
        CGFloat nextRectMaxX = CGRectGetMaxX(nextRect);
        CGFloat rectMaxXDiff = currentRectMaxX - nextRectMaxX;
        if (rectMaxXDiff > 0) {
            // Next line shorter
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMaxY(currentRect) - self.backgroundBorderRadius)];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(currentRect) - self.backgroundBorderRadius, CGRectGetMaxY(currentRect))
                       controlPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMaxY(currentRect))];
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(nextRect) + self.backgroundBorderRadius, CGRectGetMaxY(currentRect))];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(nextRect), CGRectGetMaxY(currentRect) + self.backgroundBorderRadius)
                         controlPoint:CGPointMake(CGRectGetMaxX(nextRect), CGRectGetMaxY(currentRect))];
        } else if (rectMaxXDiff < 0) {
            // Next line longer
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMinY(nextRect) - self.backgroundBorderRadius)];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(currentRect) + self.backgroundBorderRadius, CGRectGetMinY(nextRect))
                         controlPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMinY(nextRect))];
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(nextRect) - self.backgroundBorderRadius, CGRectGetMinY(nextRect))];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(nextRect), CGRectGetMinY(nextRect) + self.backgroundBorderRadius)
                         controlPoint:CGPointMake(CGRectGetMaxX(nextRect), CGRectGetMinY(nextRect))];
        } else {
            // Next line same width
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(nextRect), CGRectGetMinY(nextRect) + self.backgroundBorderRadius)];
        }
    }

    // Iterate reverse, go back up to the top left and complete the loop
    for (NSInteger i = lineRects.count - 1; i >= 0; i--) {
        CGRect currentRect = lineRects[i].CGRectValue;

        // Bottom line right line, bottom right corner, bottom line, and bottom left corner
        if (i == (NSInteger)lineRects.count - 1) {
            [path addLineToPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMaxY(currentRect) - self.backgroundBorderRadius)];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMaxX(currentRect) - self.backgroundBorderRadius, CGRectGetMaxY(currentRect))
                       controlPoint:CGPointMake(CGRectGetMaxX(currentRect), CGRectGetMaxY(currentRect))];
            [path addLineToPoint:CGPointMake(CGRectGetMinX(currentRect) + self.backgroundBorderRadius, CGRectGetMaxY(currentRect))];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMaxY(currentRect) - self.backgroundBorderRadius)
                       controlPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMaxY(currentRect))];
        }

        NSInteger nextIndex = i - 1;
        if (nextIndex < 0) {
            continue;
        }

        // Each line drawing starts right after the bottom left corner was drawn for the previous line
        // This is so the top of the current line can be adjusted based on if the next line is shorter or wider
        // If the next line is shorter, use the top of the current line
        // If the next line is wider, use the bottom of the next line
        CGRect nextRect = lineRects[nextIndex].CGRectValue;
        CGFloat currentRectMinX = CGRectGetMinX(currentRect);
        CGFloat nextRectMinX = CGRectGetMinX(nextRect);
        CGFloat rectMinXDiff = currentRectMinX - nextRectMinX;
        if (rectMinXDiff < 0) {
            // Next line shorter
            [path addLineToPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMinY(currentRect) + self.backgroundBorderRadius)];
            [path
                addQuadCurveToPoint:CGPointMake(CGRectGetMinX(currentRect) + self.backgroundBorderRadius, CGRectGetMinY(currentRect))
                       controlPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMinY(currentRect))];
            [path addLineToPoint:CGPointMake(CGRectGetMinX(nextRect) - self.backgroundBorderRadius, CGRectGetMinY(currentRect))];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMinX(nextRect), CGRectGetMinY(currentRect) - self.backgroundBorderRadius)
                         controlPoint:CGPointMake(CGRectGetMinX(nextRect), CGRectGetMinY(currentRect))];
        } else if (rectMinXDiff > 0) {
            // Next line wider
            [path addLineToPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMaxY(nextRect) + self.backgroundBorderRadius)];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMinX(currentRect) - self.backgroundBorderRadius, CGRectGetMaxY(nextRect))
                         controlPoint:CGPointMake(CGRectGetMinX(currentRect), CGRectGetMaxY(nextRect))];
            [path addLineToPoint:CGPointMake(CGRectGetMinX(nextRect) + self.backgroundBorderRadius, CGRectGetMaxY(nextRect))];
            [path addQuadCurveToPoint:CGPointMake(CGRectGetMinX(nextRect), CGRectGetMaxY(nextRect) - self.backgroundBorderRadius)
                         controlPoint:CGPointMake(CGRectGetMinX(nextRect), CGRectGetMaxY(nextRect))];
        } else {
            // Next line same width
            [path addLineToPoint:CGPointMake(CGRectGetMinX(nextRect), CGRectGetMaxY(nextRect) - self.backgroundBorderRadius)];
        }
    }

    [path closePath];
    [self.backgroundColor setFill];
    [path fill];
}

@end
