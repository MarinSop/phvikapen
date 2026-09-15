#include "core/id/Uuid.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <string>

namespace phvikapen::core {
namespace {

constexpr std::size_t kCanonicalLength = 36;

// Byte offsets in front of which the canonical form places a hyphen (8-4-4-4-12 hex digits).
constexpr std::array<std::size_t, 4> kHyphenOffsets{4, 6, 8, 10};

} // namespace

std::string Uuid::toString() const {
    std::string text;
    text.reserve(kCanonicalLength);

    for (std::size_t offset = 0; const std::uint8_t byte : m_bytes) {
        if (std::ranges::contains(kHyphenOffsets, offset)) {
            text.push_back('-');
        }
        std::format_to(std::back_inserter(text), "{:02x}", byte);
        ++offset;
    }
    return text;
}

} // namespace phvikapen::core
