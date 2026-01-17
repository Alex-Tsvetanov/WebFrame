//
//  SCValdiMainThreadDispatcher.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi_core/cpp/Interfaces/IMainThreadDispatcher.hpp"

#import <Foundation/Foundation.h>

namespace ValdiIOS {

class MainThreadDispatcher : public Valdi::IMainThreadDispatcher {
public:
    void dispatch(Valdi::DispatchFunction* function, bool sync) override;
};
} // namespace ValdiIOS

#endif
