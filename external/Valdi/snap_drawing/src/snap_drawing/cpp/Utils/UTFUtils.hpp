//
//  UTFUtils.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/16/22.
//

#pragma once

#include "snap_drawing/cpp/Text/Character.hpp"
#include <string>
#include <vector>

namespace snap::drawing {

std::vector<Character> utf8ToUnicode(std::string_view utf8);
size_t utf8ToUnicode(std::string_view utf8, std::vector<Character>& output);
std::string unicodeToUtf8(const Character* unicode, size_t length);

} // namespace snap::drawing
