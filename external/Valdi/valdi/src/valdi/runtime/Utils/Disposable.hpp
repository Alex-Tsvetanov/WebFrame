//
//  Disposable.hpp
//  valdi
//
//  Created by Simon Corsin on 5/14/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"

namespace Valdi {

class Disposable {
public:
    virtual ~Disposable() = default;

    virtual bool dispose(std::unique_lock<Mutex>& disposablesLock) = 0;
};

} // namespace Valdi
