#pragma once

#include <string>
#include <string_view>

namespace phvikapen::core {

// Plain letters, without case, diacritics or punctuation, so that a search for what a reader can
// type finds what they wrote.
[[nodiscard]] std::string folded(std::string_view text);

}
