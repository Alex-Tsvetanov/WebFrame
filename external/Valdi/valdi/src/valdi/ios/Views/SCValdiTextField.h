//
//  SCValdiTextField.h
//  Valdi
//
//  Created by Joe Engelman on 8/30/18.
//

#import <UIKit/UIKit.h>

#import "valdi/ios/Text/SCValdiAttributedTextHelper.h"
#import "valdi_core/SCValdiConfigurableTextHolder.h"
#import "valdi_core/UIView+ValdiBase.h"

@interface SCValdiTextField : UITextField <UITextFieldDelegate, SCValdiConfigurableTextHolder, SCValdiTextHolder>

@end
