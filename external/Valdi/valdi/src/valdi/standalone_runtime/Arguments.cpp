//
//  Arguments.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 1/28/20.
//

#include "valdi/standalone_runtime/Arguments.hpp"
#include "utils/debugging/Assert.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <algorithm>

namespace Valdi {

Arguments::Arguments(int argc, const char** argv) : _argv(argv, argv + argc) {}

const char* const* Arguments::argv() const {
    return _argv.data();
}

int Arguments::argc() const {
    return static_cast<int>(_argv.size());
}

StringBox Arguments::next() {
    SC_ASSERT(hasNext());
    return StringCache::makeStringFromCLiteral(_argv[_i++]);
}

bool Arguments::hasNext() const {
    return _i < argc();
}

void Arguments::consumeLast() {
    _argv.erase(_argv.begin() + (--_i));
}

void Arguments::rewind() {
    _i = 0;
}

} // namespace Valdi
