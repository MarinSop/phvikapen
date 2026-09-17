#include "core/id/ContentId.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace phvikapen::core {
namespace {

constexpr std::array<char, 16> kDigits{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};
constexpr unsigned kHighNibbleShift = 4;
constexpr unsigned kLowNibbleMask = 0x0F;

}

std::string ContentId::toString() const {
    std::string text;
    text.reserve(kByteCount * 2);
    for (const std::uint8_t byte : m_bytes) {
        text.push_back(kDigits.at(static_cast<unsigned>(byte) >> kHighNibbleShift));
        text.push_back(kDigits.at(static_cast<unsigned>(byte) & kLowNibbleMask));
    }
    return text;
}

}
