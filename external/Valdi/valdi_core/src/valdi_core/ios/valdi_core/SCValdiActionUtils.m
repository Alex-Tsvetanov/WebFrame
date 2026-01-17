//
//  SCValdiActionUtils.m
//  Valdi
//
//  Created by Simon Corsin on 5/7/18.
//

#import "valdi_core/SCValdiActionUtils.h"

#import "valdi_core/SCValdiAction.h"

NSArray *SCValdiActionParametersWithSender(id sender)
{
    if (sender) {
        return @[ @{SCValdiActionSenderKey : sender} ];
    } else {
        return @[];
    }
}
