//
//  SCValdiEventTime.m
//  valdi-ios
//
//  Created by Simon Corsin on 9/5/19.
//

#import "SCValdiEventTime.h"
#import "valdi_core/cpp/Utils/TimePoint.hpp"

NSTimeInterval SCValdiGetCurrentEventTime() {
    return Valdi::TimePoint::now().getTime();
}
