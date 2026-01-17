//
//  Arguments.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 1/28/20.
//

#pragma once

#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class Arguments {
public:
    Arguments(int argc, const char** argv);

    int argc() const;
    const char* const* argv() const;

    StringBox next();
    bool hasNext() const;

    void consumeLast();

    void rewind();

private:
    int _i = 0;
    std::vector<const char*> _argv;
};

} // namespace Valdi
