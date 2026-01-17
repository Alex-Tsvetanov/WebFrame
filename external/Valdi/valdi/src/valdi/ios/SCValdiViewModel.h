//
//  SCValdiViewModel.h
//  iOS
//
//  Created by Simon Corsin on 4/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol SCValdiViewModel <NSObject, NSCopying>

- (NSDictionary<NSString*, id>*)toJavaScript;

@end
