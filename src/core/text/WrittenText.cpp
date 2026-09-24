#include "core/text/WrittenText.hpp"

#include "core/geometry/Rect.hpp"
#include "core/model/TextBox.hpp"
#include "core/text/InkWord.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

// Two words belong to the same line when they stand beside one another rather than below: they
// share this much of the shorter one's height.
constexpr float kSameLine = 0.5F;
// Most words are shorter than the letters that reach highest and lowest, so the size of type is
// taken from the taller words rather than the middle one.
constexpr float kTallWords = 0.75F;

[[nodiscard]] bool besideEachOther(const Rect& line, const Rect& word) noexcept {
    const float overlap = std::min(line.bottom, word.bottom) - std::max(line.top, word.top);
    const float shorter = std::min(line.height(), word.height());
    return shorter > 0.0F ? overlap > shorter * kSameLine : overlap > 0.0F;
}

[[nodiscard]] std::vector<std::vector<InkWord>> linesOf(std::span<const InkWord> words) {
    std::vector<InkWord> inOrder{words.begin(), words.end()};
    std::ranges::stable_sort(inOrder, {}, [](const InkWord& word) { return word.box.top; });

    std::vector<std::vector<InkWord>> lines;
    Rect running{};
    for (const InkWord& word : inOrder) {
        if (lines.empty() || !besideEachOther(running, word.box)) {
            lines.push_back({word});
            running = word.box;
            continue;
        }
        lines.back().push_back(word);
        running = running.united(word.box);
    }
    for (std::vector<InkWord>& line : lines) {
        std::ranges::stable_sort(line, {}, [](const InkWord& word) { return word.box.left; });
    }
    return lines;
}

[[nodiscard]] float sizeOf(std::span<const InkWord> words) {
    std::vector<float> heights;
    heights.reserve(words.size());
    for (const InkWord& word : words) {
        heights.push_back(word.box.height());
    }
    std::ranges::sort(heights);
    const auto tall =
        static_cast<std::size_t>(std::lround(static_cast<float>(heights.size() - 1) * kTallWords));
    return std::clamp(pointsOfPageUnits(heights[tall]), TextStyle::kSmallestSize,
                      TextStyle::kLargestSize);
}

}

std::optional<TextBlock> textOf(std::span<const InkWord> words) {
    if (words.empty()) {
        return std::nullopt;
    }

    std::string text;
    Rect area = words.front().box;
    for (const std::vector<InkWord>& line : linesOf(words)) {
        if (!text.empty()) {
            text.push_back('\n');
        }
        bool first = true;
        for (const InkWord& word : line) {
            if (!first) {
                text.push_back(' ');
            }
            text.append(word.text);
            area = area.united(word.box);
            first = false;
        }
    }
    if (text.empty()) {
        return std::nullopt;
    }

    return TextBlock{
        .text = text,
        .area = area,
        .width = std::max(area.width() * kRoomToBreathe, TextBox::kNarrowest),
        .size = sizeOf(words),
    };
}

}
