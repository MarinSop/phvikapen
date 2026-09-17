#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>

namespace phvikapen::core {

class ContentId {
public:
    static constexpr std::size_t kByteCount = 32;

    using Bytes = std::array<std::uint8_t, kByteCount>;

    constexpr ContentId() noexcept = default;

    constexpr explicit ContentId(const Bytes& bytes) noexcept : m_bytes{bytes} {}

    [[nodiscard]] constexpr const Bytes& bytes() const noexcept { return m_bytes; }

    [[nodiscard]] constexpr bool isEmpty() const noexcept { return m_bytes == Bytes{}; }

    [[nodiscard]] std::string toString() const;

    friend constexpr std::strong_ordering operator<=>(const ContentId&,
                                                      const ContentId&) noexcept = default;

    friend constexpr bool operator==(const ContentId&, const ContentId&) noexcept = default;

private:
    Bytes m_bytes{};
};

}
