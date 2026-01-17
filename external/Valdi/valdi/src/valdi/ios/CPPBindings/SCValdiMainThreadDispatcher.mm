//
//  SCValdiMainThreadDispatcher.m
//  ValdiIOS
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/CPPBindings/SCValdiMainThreadDispatcher.h"

namespace ValdiIOS
{

void mainThreadEntry(void* context) {
    Valdi::DispatchFunction *function = reinterpret_cast<Valdi::DispatchFunction *>(context);
    (*function)();

    delete function;
}

void MainThreadDispatcher::dispatch(Valdi::DispatchFunction *function, bool sync)
{
    if (sync) {
        dispatch_sync_f(dispatch_get_main_queue(), function, &mainThreadEntry);
    } else {
        dispatch_async_f(dispatch_get_main_queue(), function, &mainThreadEntry);
    }
}

}
