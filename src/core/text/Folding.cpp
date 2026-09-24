#include "core/text/Folding.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace phvikapen::core {
namespace {

constexpr char32_t kAsciiEnd = 0x80;
constexpr char32_t kLatinExtendedEnd = 0x180;
constexpr std::uint8_t kContinuationMask = 0xC0;
constexpr std::uint8_t kContinuation = 0x80;
constexpr std::uint8_t kPayloadMask = 0x3F;
constexpr std::uint8_t kTwoByteMark = 0xE0;
constexpr std::uint8_t kTwoByte = 0xC0;
constexpr std::uint8_t kThreeByteMark = 0xF0;
constexpr std::uint8_t kThreeByte = 0xE0;
constexpr std::uint8_t kFourByteMark = 0xF8;
constexpr std::uint8_t kFourByte = 0xF0;
constexpr unsigned kPayloadBits = 6;
constexpr std::uint8_t kTwoByteBits = 0x1F;
constexpr std::uint8_t kThreeByteBits = 0x0F;
constexpr std::uint8_t kFourByteBits = 0x07;

struct Letter {
    char32_t code{};
    std::size_t length{1};
};

[[nodiscard]] Letter letterAt(std::string_view text, std::size_t at) noexcept {
    const auto lead = static_cast<std::uint8_t>(text[at]);
    const auto follows = [&](std::size_t offset) {
        return at + offset < text.size()
               && (static_cast<std::uint8_t>(text[at + offset]) & kContinuationMask)
                      == kContinuation;
    };
    const auto payload = [&](std::size_t offset) {
        return static_cast<char32_t>(static_cast<std::uint8_t>(text[at + offset]) & kPayloadMask);
    };
    if (lead < kAsciiEnd) {
        return {.code = lead, .length = 1};
    }
    if ((lead & kTwoByteMark) == kTwoByte && follows(1)) {
        return {
            .code = (static_cast<char32_t>(lead & kTwoByteBits) << kPayloadBits) | payload(1),
            .length = 2,
        };
    }
    if ((lead & kThreeByteMark) == kThreeByte && follows(1) && follows(2)) {
        return {
            .code = (static_cast<char32_t>(lead & kThreeByteBits) << (2 * kPayloadBits))
                    | (payload(1) << kPayloadBits) | payload(2),
            .length = 3,
        };
    }
    if ((lead & kFourByteMark) == kFourByte && follows(1) && follows(2) && follows(3)) {
        return {
            .code = (static_cast<char32_t>(lead & kFourByteBits) << (3 * kPayloadBits))
                    | (payload(1) << (2 * kPayloadBits)) | (payload(2) << kPayloadBits)
                    | payload(3),
            .length = 4,
        };
    }
    return {.code = lead, .length = 1};
}

struct Range {
    char32_t from{};
    char32_t to{};
    std::string_view plain;
};

// The letters of Latin writing, each range folded down to the letters a reader would type.
constexpr std::array kPlainLetters{
    Range{.from = 0xC0, .to = 0xC5, .plain = "a"},
    Range{.from = 0xC6, .to = 0xC6, .plain = "ae"},
    Range{.from = 0xC7, .to = 0xC7, .plain = "c"},
    Range{.from = 0xC8, .to = 0xCB, .plain = "e"},
    Range{.from = 0xCC, .to = 0xCF, .plain = "i"},
    Range{.from = 0xD0, .to = 0xD0, .plain = "d"},
    Range{.from = 0xD1, .to = 0xD1, .plain = "n"},
    Range{.from = 0xD2, .to = 0xD6, .plain = "o"},
    Range{.from = 0xD8, .to = 0xD8, .plain = "o"},
    Range{.from = 0xD9, .to = 0xDC, .plain = "u"},
    Range{.from = 0xDD, .to = 0xDD, .plain = "y"},
    Range{.from = 0xDF, .to = 0xDF, .plain = "ss"},
    Range{.from = 0xE0, .to = 0xE5, .plain = "a"},
    Range{.from = 0xE6, .to = 0xE6, .plain = "ae"},
    Range{.from = 0xE7, .to = 0xE7, .plain = "c"},
    Range{.from = 0xE8, .to = 0xEB, .plain = "e"},
    Range{.from = 0xEC, .to = 0xEF, .plain = "i"},
    Range{.from = 0xF0, .to = 0xF0, .plain = "d"},
    Range{.from = 0xF1, .to = 0xF1, .plain = "n"},
    Range{.from = 0xF2, .to = 0xF6, .plain = "o"},
    Range{.from = 0xF8, .to = 0xF8, .plain = "o"},
    Range{.from = 0xF9, .to = 0xFC, .plain = "u"},
    Range{.from = 0xFD, .to = 0xFD, .plain = "y"},
    Range{.from = 0xFF, .to = 0xFF, .plain = "y"},
    Range{.from = 0x100, .to = 0x105, .plain = "a"},
    Range{.from = 0x106, .to = 0x10D, .plain = "c"},
    Range{.from = 0x10E, .to = 0x111, .plain = "d"},
    Range{.from = 0x112, .to = 0x11B, .plain = "e"},
    Range{.from = 0x11C, .to = 0x123, .plain = "g"},
    Range{.from = 0x124, .to = 0x127, .plain = "h"},
    Range{.from = 0x128, .to = 0x131, .plain = "i"},
    Range{.from = 0x132, .to = 0x133, .plain = "ij"},
    Range{.from = 0x134, .to = 0x135, .plain = "j"},
    Range{.from = 0x136, .to = 0x138, .plain = "k"},
    Range{.from = 0x139, .to = 0x142, .plain = "l"},
    Range{.from = 0x143, .to = 0x14B, .plain = "n"},
    Range{.from = 0x14C, .to = 0x151, .plain = "o"},
    Range{.from = 0x152, .to = 0x153, .plain = "oe"},
    Range{.from = 0x154, .to = 0x159, .plain = "r"},
    Range{.from = 0x15A, .to = 0x161, .plain = "s"},
    Range{.from = 0x162, .to = 0x167, .plain = "t"},
    Range{.from = 0x168, .to = 0x173, .plain = "u"},
    Range{.from = 0x174, .to = 0x175, .plain = "w"},
    Range{.from = 0x176, .to = 0x178, .plain = "y"},
    Range{.from = 0x179, .to = 0x17E, .plain = "z"},
    Range{.from = 0x17F, .to = 0x17F, .plain = "s"},
};

[[nodiscard]] std::string_view plainLetters(char32_t code) {
    for (const Range& range : kPlainLetters) {
        if (code >= range.from && code <= range.to) {
            return range.plain;
        }
    }
    return {};
}

}

std::string folded(std::string_view text) {
    std::string plain;
    plain.reserve(text.size());
    for (std::size_t at = 0; at < text.size();) {
        const Letter letter = letterAt(text, at);
        at += letter.length;
        const char32_t code = letter.code;
        if (code < kAsciiEnd) {
            const auto narrow = static_cast<char>(code);
            if ((narrow >= '0' && narrow <= '9') || (narrow >= 'a' && narrow <= 'z')) {
                plain.push_back(narrow);
            } else if (narrow >= 'A' && narrow <= 'Z') {
                plain.push_back(static_cast<char>(narrow - 'A' + 'a'));
            }
            continue;
        }
        plain.append(code < kLatinExtendedEnd ? plainLetters(code) : std::string_view{});
    }
    return plain;
}

}
