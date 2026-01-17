// Copyright © 2024 Snap, Inc. All rights reserved.

#import <Foundation/Foundation.h>
#import "valdi/ios/Text/SCValdiAttributedTextHelper.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/UIColor+Valdi.h"

static const NSInteger SCTextShadowParameterCount = 5;

BOOL SCValdiNeedAttributedString(UIView<SCValdiTextAttributable> *textHolder, SCValdiFontAttributes *fontAttributes)
{

    if (fontAttributes.needAttributedString) {
        return YES;
    }

    if (textHolder.textValue && (![textHolder.textValue isKindOfClass:[NSString class]])) {
        return YES;
    }

    return NO;
}

void SCValdiClearAttributedText(UIView<SCValdiTextHolder> *textHolder)
{
    // There's a UIKit bug, where the label caches a set of "default attributes"
    // based on the `attributedText` that was previously set on it. Setting `attributedText`
    // to nil does not seem to clear the paragraphStyle from these cached attributes.
    // Calling the private API -[UILabel _setDefaultAttributes:nil] seem to help,
    // but in the interest of avoiding using a private system API we looked for an
    // alternative approach.
    //
    // Setting an attributed string value with an explicit default paragraph style,
    // then setting a nil `text` before setting a nil `attributedText` seems to do it.
    static dispatch_once_t onceToken;
    static NSAttributedString *cleanupAttributedString;
    dispatch_once(&onceToken, ^{
        NSDictionary *cleanupAttributes = @{
            NSParagraphStyleAttributeName: NSParagraphStyle.defaultParagraphStyle
        };
        // This doesn't seem to work correctly with an empty string value
        cleanupAttributedString = [[NSAttributedString alloc] initWithString:@"-"
                                                                  attributes:cleanupAttributes];
    });
    textHolder.attributedText = cleanupAttributedString;
    textHolder.text = nil;

    textHolder.attributedText = nil;
}


BOOL SCValdiUpdateLabelMode(UIView<SCValdiTextAttributable> *textAttributable, UIView<SCValdiTextHolder> *textHolder, SCValdiTextMode labelMode)
{
    if (textAttributable.textMode == labelMode) {
        return NO;
    }

    // Cleanup
    switch (textAttributable.textMode) {
        case SCValdiTextModeText:
            textHolder.text = nil;
            break;
        case SCValdiTextModeAttributedText:
            SCValdiClearAttributedText(textHolder);
            break;
        case SCValdiTextModeValdiTextLayout:
            // noop - no support for touch events in SCValdiTextField
            break;
    }

    // Setup
    textAttributable.textMode = labelMode;
    return YES;
}

void setNeedsLayoutRecursive(UIView *view) {
    [view setNeedsLayout];

    for (UIView *subview in view.subviews) {
        setNeedsLayoutRecursive(subview);
    }
}

void SCValdiSetTextHolderAttributes(UIView<SCValdiTextHolder> *textHolder, SCValdiFontAttributes *fontAttributes, UITraitCollection *traitCollection, BOOL isRightToLeft, UIColor *textColor)
{
    UIFont *font = [fontAttributes.font resolveFontFromTraitCollection:traitCollection];
    if (textHolder.font != font) {
        textHolder.font = font;
        // Workaround a UITextView issue where changing the font doesn't properly make the text view
        // layout if the text view has no text
        setNeedsLayoutRecursive(textHolder);
    }

    if (textHolder.textColor != textColor && textColor != nil) {
        textHolder.textColor = textColor;
    }

    NSTextAlignment resolvedTextAlignment = [fontAttributes resolveTextAlignmentWithIsRightToLeft:isRightToLeft];
    if (textHolder.textAlignment != resolvedTextAlignment) {
        textHolder.textAlignment = resolvedTextAlignment;
        [textHolder setNeedsLayout];
    }
}

NSString* SCValdiClampTextValueChanged(NSString *rawValue, NSString *replacementText, NSRange range, NSInteger characterLimit, BOOL ignoreNewlines)
{
    NSString *value = [rawValue stringByReplacingCharactersInRange:range withString:replacementText];

    // Remove newlines already in the text
    if (ignoreNewlines && [value rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].length) {
        value = [[value componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]]
                 componentsJoinedByString:@""];
    }

    // Clamp up to the character limit if needed
    if (characterLimit > 0 && value.length > (NSUInteger)characterLimit) {
        NSInteger charactersRemaining = MAX(0, characterLimit - rawValue.length);
        NSString *remainingTextToInsert = [replacementText substringWithRange:NSMakeRange(0, MIN(replacementText.length, charactersRemaining))];
        value = [rawValue stringByReplacingCharactersInRange:range withString:remainingTextToInsert];
    }

    return value;
}

NSString* SCValdiClampTextValue(NSString *rawValue, NSInteger characterLimit, BOOL ignoreNewlines)
{
    NSString *value = rawValue;

    // Remove newlines already in the text
    if (ignoreNewlines && [value rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].length) {
        value = [[value componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]]
                 componentsJoinedByString:@""];
    }

    // Clamp to character limit if needed
    if (characterLimit > 0 && value.length > (NSUInteger)characterLimit) {
        value = [value substringWithRange:NSMakeRange(0, characterLimit)];
    }

    return value;
}

NSAttributedString* SCValdiClampAttributedStringValue(NSAttributedString *rawValue, NSInteger characterLimit, BOOL ignoreNewlines)
{
    NSAttributedString *value = rawValue;
    // clamp to character limit if needed
    if (characterLimit != 0 && characterLimit >= 0 && value.length > (NSUInteger)characterLimit) {
        value = [NSAttributedString trimAttributedString:value characterLimit:characterLimit];
    }

    // strip out newline characters
    if (ignoreNewlines && [value.string rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].length) {
        NSMutableAttributedString *filteredAttributedString = [[NSMutableAttributedString alloc] init];
        [value.string enumerateSubstringsInRange:NSMakeRange(0, value.length) options:NSStringEnumerationByComposedCharacterSequences usingBlock:^(NSString * _Nullable substring, NSRange substringRange, NSRange enclosingRange, BOOL * _Nonnull stop) {
            if ([substring rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].location == NSNotFound) {
                NSAttributedString *attributedSubstring = [value attributedSubstringFromRange:substringRange];
                [filteredAttributedString appendAttributedString:attributedSubstring];
            }
        }];
        value = filteredAttributedString;
    }

    return value;
}


BOOL SCValdiSetTextHolderTextShadow(UIView<SCValdiTextHolder>* textHolder,
                                       NSArray* textShadow)
{
    if (textShadow.count != SCTextShadowParameterCount) {
        SCLogValdiError(@"Invalid text shadow options");
        return NO;
    }

    UIColor *shadowColor =
                UIColorFromValdiAttributeValue((int64_t)[[textShadow objectAtIndex:0] integerValue]);
    textHolder.layer.shadowColor = [shadowColor CGColor];

    textHolder.layer.shadowRadius = [[textShadow objectAtIndex:1] doubleValue];
    textHolder.layer.shadowOpacity = [[textShadow objectAtIndex:2] doubleValue];
    textHolder.layer.shadowOffset = CGSizeMake([[textShadow objectAtIndex:3] doubleValue], [[textShadow objectAtIndex:4] doubleValue]);
    return YES;
}


void SCValdiResetTextHolderTextShadow(UIView<SCValdiTextHolder>* textHolder)
{
    textHolder.layer.shadowPath = nil;
    textHolder.layer.shadowColor = nil;
    textHolder.layer.shadowRadius = 0;
    textHolder.layer.shadowOpacity = 0;
    textHolder.layer.shadowOffset = CGSizeMake(0, 0);
}
