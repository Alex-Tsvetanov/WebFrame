
#import "valdi/ios/Text/SCValdiFont.h"
#import "valdi_core/UIColor+Valdi.h"
#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/SCValdiFontManagerProtocol.h"

@implementation SCValdiFont {
    UIFont *_font;
    UIFontTextStyle _textStyle;
    CGFloat _maxSize;
    UIFont *_lastFont;
    UITraitCollection *_lastTraitCollection;
    id<SCValdiFontManagerProtocol> _fontManager;
}

- (instancetype)initWithFont:(UIFont *)font fontManager:(id<SCValdiFontManagerProtocol>)fontManager
{
    self = [self init];
    if (self) {
        _font = font;
        _textStyle = UIFontTextStyleBody;
        _maxSize = 0;
        _lastFont = nil;
        _lastTraitCollection = nil;
        _fontManager = fontManager;
    }
    return self;
}

- (instancetype)initWithFont:(UIFont *)font
                   textStyle:(UIFontTextStyle)textStyle
                     maxSize:(CGFloat)maxSize
                     fontManager:(id<SCValdiFontManagerProtocol>)fontManager
{
    self = [self init];
    if (self) {
        _font = font;
        _textStyle = textStyle;
        _maxSize = maxSize;
        _lastFont = nil;
        _lastTraitCollection = nil;
        _fontManager = fontManager;
    }
    return self;
}

- (id<SCValdiFontManagerProtocol>)fontManager
{
    return _fontManager;
}

- (UIFont *)resolveFontFromTraitCollection:(UITraitCollection *)traitCollection
{
    if (traitCollection == nil) {
        traitCollection = [_fontManager defaultTraitCollection];
    }
    if (@available(iOS 11.0, *)) {
        @synchronized (self) {
            if (traitCollection == _lastTraitCollection && _lastFont != nil) {
                return _lastFont;
            }
            if (_font != nil && _textStyle != nil) {
                UIFontMetrics *metrics = [UIFontMetrics metricsForTextStyle:_textStyle];
                UITraitCollection* traits = traitCollection;
                UIFont* fontToResolve = _font;
                if ([_fontManager shouldBypassContextForLegibilityWeight]) {
                    // Hardcode the UILegibilityWeight trait to 'regular' and rely on the font loader to return a font
                    // correctly resolved for this..
                    if (@available(iOS 13.0, *)) {
                        UITraitCollection* legibilityOverride =
                            [UITraitCollection traitCollectionWithLegibilityWeight:UILegibilityWeightRegular];
                        traits = [UITraitCollection
                            traitCollectionWithTraitsFromCollections:@[ traitCollection, legibilityOverride ]];
                        if (traitCollection.legibilityWeight == UILegibilityWeightBold) {
                            fontToResolve = [_fontManager fontWithName:_font.fontName
                                                              fontSize:_font.pointSize
                                                       legibilityWeight:SCUILegibilityWeightBold];
                        }
                    }
                }
                if (_maxSize > 0) {
                    _lastFont = [metrics scaledFontForFont:fontToResolve
                                          maximumPointSize:_maxSize
                             compatibleWithTraitCollection:traits];
                } else {
                    _lastFont = [metrics scaledFontForFont:fontToResolve compatibleWithTraitCollection:traits];
                }
                _lastTraitCollection = traitCollection;
                return _lastFont;
            }
        }
    }
    return _font;
}

+ (SCValdiFont *)fontFromValdiAttribute:(NSString *)attributeValue fontManager:(id<SCValdiFontManagerProtocol>)fontManager
{
    if (attributeValue == nil) {
        return [[SCValdiFont alloc] initWithFont:[UIFont systemFontOfSize:[UIFont labelFontSize]] fontManager:fontManager];
    }

    NSArray *components = [attributeValue componentsSeparatedByString:@" "];

    if (components.count < 2) {
        UIFontTextStyle fontStyle = [SCValdiFont fontTextStyleFromString:attributeValue];
        if (fontStyle != nil) {
            return [[SCValdiFont alloc] initWithFont:[UIFont preferredFontForTextStyle:fontStyle] fontManager:fontManager];
        }
        SCLogValdiError(@"Font attribute should be formatted as follows: '<fontName> <fontSize> <fontStyle?> <maxFontSize?>'");
        return nil;
    }

    NSString *fontName = components[0];
    CGFloat fontSize = [components[1] doubleValue];

    UIFontTextStyle textStyle = nil;
    if (components.count >= 3) {
        textStyle = [SCValdiFont fontTextStyleFromString:components[2]];
    }
    CGFloat maxSize = 0;
    if (components.count >= 4) {
        maxSize = [components[3] doubleValue];
    }

    UIFont *font = [fontManager fontWithName:fontName fontSize:fontSize legibilityWeight:SCUILegibilityWeightRegular];

    return [[SCValdiFont alloc] initWithFont:font textStyle:textStyle maxSize:maxSize fontManager:fontManager];
}

+ (UIFontTextStyle)fontTextStyleFromString:(NSString *)fontTextStyleStr
{
    if (fontTextStyleStr == nil) {
        return UIFontTextStyleBody;
    }
    NSString* fontTextStyleLowercase = [fontTextStyleStr lowercaseString];
    if ([fontTextStyleLowercase isEqualToString:@"body"]) {
        return UIFontTextStyleBody;
    } else if ([fontTextStyleLowercase isEqualToString:@"callout"]) {
        return UIFontTextStyleCallout;
    } else if ([fontTextStyleLowercase isEqualToString:@"caption1"]) {
        return UIFontTextStyleCaption1;
    } else if ([fontTextStyleLowercase isEqualToString:@"caption2"]) {
        return UIFontTextStyleCaption2;
    } else if ([fontTextStyleLowercase isEqualToString:@"footnote"]) {
        return UIFontTextStyleFootnote;
    } else if ([fontTextStyleLowercase isEqualToString:@"headline"]) {
        return UIFontTextStyleHeadline;
    } else if ([fontTextStyleLowercase isEqualToString:@"subheadline"]) {
        return UIFontTextStyleSubheadline;
    } else if ([fontTextStyleLowercase isEqualToString:@"largetitle"]) {
        if (@available(iOS 11.0, *)) {
            return UIFontTextStyleLargeTitle;
        }
    } else if ([fontTextStyleLowercase isEqualToString:@"title1"]) {
        return UIFontTextStyleTitle1;
    } else if ([fontTextStyleLowercase isEqualToString:@"title2"]) {
        return UIFontTextStyleTitle2;
    } else if ([fontTextStyleLowercase isEqualToString:@"title3"]) {
        return UIFontTextStyleTitle3;
    } else if ([fontTextStyleLowercase isEqualToString:@"unscaled"]) {
        return nil; // special case where we want to force that no scaling is applied
    }
    return UIFontTextStyleBody;
}

@end
