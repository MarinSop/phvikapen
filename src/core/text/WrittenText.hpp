#pragma once

#include "core/geometry/Rect.hpp"
#include "core/text/InkWord.hpp"

#include <optional>
#include <span>
#include <string>

namespace phvikapen::core {

// What handwriting says, laid out as lines of text: the words in reading order, where they sat on
// the page, and the size of type that matches the hand that wrote them.
struct TextBlock {
    std::string text;
    Rect area;
    float width{};
    float size{};

    friend bool operator==(const TextBlock&, const TextBlock&) = default;
};

// A little more room than the writing took, so that type of about the same size still fits.
inline constexpr float kRoomToBreathe = 1.1F;

[[nodiscard]] std::optional<TextBlock> textOf(std::span<const InkWord> words);

}
