#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace phvikapen::core {

// Plain letters, without case, diacritics or punctuation, so that a search for what a reader can
// type finds what they wrote.
[[nodiscard]] std::string folded(std::string_view text);

// How short an asking has to be before nothing is forgiven in it. Below this, one letter in three
// would make a different word altogether.
inline constexpr std::size_t kForgivingFrom = 5;

// How much of an asking is enough to look it up by, where the rest may be misread.
inline constexpr std::size_t kStemLength = 4;

// Whether a word, already plain, reads as what was asked for, also plain. A word the asking is
// found inside reads as it; so does one a single letter away from it, because a reader of
// handwriting mistakes about that many. Nothing is forgiven in an asking shorter than
// `kForgivingFrom`.
[[nodiscard]] bool readsAs(std::string_view word, std::string_view wanted);

// The part of a plain asking a word can be looked up by, leaving the rest to be misread.
[[nodiscard]] std::string_view stemOf(std::string_view wanted);

}
