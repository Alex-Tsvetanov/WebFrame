//
//  SCValdiViewNode.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/25/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/SCValdiContext.h"
#import "valdi_core/SCValdiRectUtils.h"

#import "valdi_core/SCValdiViewNodeProtocol.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface SCValdiViewNode : NSObject <SCValdiViewNodeProtocol>

@property (readonly, nonatomic, weak) UIView* view;

@end
