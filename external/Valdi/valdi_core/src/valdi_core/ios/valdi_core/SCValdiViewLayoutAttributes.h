//
//  SCValdiViewLayoutAttributes.h
//  valdi_core
//
//  Created by Simon Corsin on 6/14/22.
//

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@protocol SCValdiViewLayoutAttributes <NSObject>

- (id)valueForAttributeName:(NSString*)attributeName;
- (BOOL)boolValueForAttributeName:(NSString*)attributeName;
- (NSString*)stringValueForAttributeName:(NSString*)attributeName;
- (CGFloat)doubleValueForAttributeName:(NSString*)attributeName;

@end
