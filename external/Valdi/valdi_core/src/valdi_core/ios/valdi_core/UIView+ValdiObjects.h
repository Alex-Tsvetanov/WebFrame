//
//  UIView+ValdiObjects.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/25/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiContextProtocol.h"
#import "valdi_core/SCValdiViewNodeProtocol.h"

#import <UIKit/UIKit.h>

@interface UIView (ValdiObjects)

@property (strong, nonatomic) id<SCValdiContextProtocol> valdiContext;

@property (strong, nonatomic) id<SCValdiViewNodeProtocol> valdiViewNode;

@end
