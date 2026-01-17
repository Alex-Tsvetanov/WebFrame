//
//  IQueueListener.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/16/19.
//

#pragma once

namespace Valdi {

class IQueueListener {
public:
    virtual ~IQueueListener() = default;

    virtual void onQueueEmpty() = 0;
    virtual void onQueueNonEmpty() = 0;
};

} // namespace Valdi
