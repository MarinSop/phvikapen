#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>

namespace phvikapen::core {

class Uuid {
public:
    static constexpr std::size_t kByteCount = 16;

    using Bytes = std::array<std::uint8_t, kByteCount>;

    constexpr Uuid() noexcept = default;

    constexpr explicit Uuid(const Bytes& bytes) noexcept : m_bytes{bytes} {}

    [[nodiscard]] constexpr const Bytes& bytes() const noexcept { return m_bytes; }

    [[nodiscard]] constexpr bool isNil() const noexcept { return m_bytes == Bytes{}; }

    [[nodiscard]] constexpr int version() const noexcept {
        return static_cast<int>(static_cast<unsigned>(m_bytes[kVersionByte]) >> kVersionShift);
    }

    [[nodiscard]] std::string toString() const;

    friend constexpr std::strong_ordering operator<=>(const Uuid&, const Uuid&) noexcept = default;

    friend constexpr bool operator==(const Uuid&, const Uuid&) noexcept = default;

private:
    static constexpr std::size_t kVersionByte = 6;
    static constexpr unsigned kVersionShift = 4;

    Bytes m_bytes{};
};

}
