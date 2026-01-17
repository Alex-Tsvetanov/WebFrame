//
//  SCValdiAttributedTextHelper.h
//  Valdi
//
//  Created by David Li on 07/04/24.
//  Copyright © 2024 Snap Inc. All rights reserved.
//
#import "valdi/ios/Text/NSAttributedString+Valdi.h"
#import "valdi/ios/Text/SCValdiFontAttributes.h"
#import "valdi_core/SCValdiConfigurableTextHolder.h"

// For SCValdiLabel and SCValdiTextField, both are TextAttributable as well as TextHolder
@protocol SCValdiTextAttributable <NSObject>

@property (nonatomic, strong) id textValue;
@property (nonatomic) SCValdiTextMode textMode;

@end

// however, we keep text specific attributes separately, as SCValdiTextView contains two nested UITextViews
// this is to support the placeholder + textview
@protocol SCValdiTextHolder <NSObject, SCValdiConfigurableTextHolder>

@property (nonatomic, strong) NSString* text;
@property (nonatomic, strong) NSAttributedString* attributedText;
@property (nonatomic, strong) UIFont* font;
@property (nonatomic, strong) UIColor* textColor;

@end

BOOL SCValdiNeedAttributedString(UIView<SCValdiTextAttributable>* textHolder, SCValdiFontAttributes* fontAttributes);

void SCValdiClearAttributedText(UIView<SCValdiTextHolder>* textHolder);

BOOL SCValdiUpdateLabelMode(UIView<SCValdiTextAttributable>* textAttributable,
                            UIView<SCValdiTextHolder>* textHolder,
                            SCValdiTextMode labelMode);

void SCValdiSetTextHolderAttributes(UIView<SCValdiTextHolder>* textHolder,
                                    SCValdiFontAttributes* fontAttributes,
                                    UITraitCollection* traitCollection,
                                    BOOL isRightToLeft,
                                    UIColor* textColor);

NSString* SCValdiClampTextValueChanged(
    NSString* rawValue, NSString* replacementText, NSRange range, NSInteger characterLimit, BOOL ignoreNewlines);

NSString* SCValdiClampTextValue(NSString* rawValue, NSInteger characterLimit, BOOL ignoreNewlines);

NSAttributedString* SCValdiClampAttributedStringValue(NSAttributedString* rawValue,
                                                      NSInteger characterLimit,
                                                      BOOL ignoreNewlines);

BOOL SCValdiSetTextHolderTextShadow(UIView<SCValdiTextHolder>* textHolder, NSArray* textShadow);
void SCValdiResetTextHolderTextShadow(UIView<SCValdiTextHolder>* textHolder);
