//
//  Void.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#pragma once

namespace Valdi {

/**
 An empty type that can be used as a placeholder for void in a template
 */
struct Void {
    Void() = default;

    template<class... T>
    explicit Void(T... rest) {}
};
} // namespace Valdi
