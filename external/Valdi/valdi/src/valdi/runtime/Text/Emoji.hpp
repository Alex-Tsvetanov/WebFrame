#pragma once

#include "valdi_core/cpp/Text/UnicodeSequenceTrie.hpp"
#include <vector>

namespace Valdi {

/**
Returns a unicode trie set that contains all the emoji characters.
 */
const UnicodeSequenceTrie* getEmojiUnicodeTrie();

} // namespace Valdi