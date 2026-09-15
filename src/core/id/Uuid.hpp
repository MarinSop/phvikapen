#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>

namespace phvikapen::core {

/// 128-bit universally unique identifier (RFC 9562), stored in network byte order.
///
/// Ordering compares the bytes lexicographically, which for UUIDv7 follows creation time.
class Uuid {
public:
    static constexpr std::size_t kByteCount = 16;

    using Bytes = std::array<std::uint8_t, kByteCount>;

    /// Constructs the nil UUID.
    constexpr Uuid() noexcept = default;

    constexpr explicit Uuid(const Bytes& bytes) noexcept : m_bytes{bytes} {}

    [[nodiscard]] constexpr const Bytes& bytes() const noexcept { return m_bytes; }

    [[nodiscard]] constexpr bool isNil() const noexcept { return m_bytes == Bytes{}; }

    /// Version field (RFC 9562 section 4.2), for example 7 for time-ordered UUIDs.
    [[nodiscard]] constexpr int version() const noexcept {
        return static_cast<int>(static_cast<unsigned>(m_bytes[kVersionByte]) >> kVersionShift);
    }

    /// Canonical lowercase form, for example "01890a5d-ac96-774b-bcce-b302099a8057".
    [[nodiscard]] std::string toString() const;

    friend constexpr std::strong_ordering operator<=>(const Uuid&, const Uuid&) noexcept = default;

    friend constexpr bool operator==(const Uuid&, const Uuid&) noexcept = default;

private:
    static constexpr std::size_t kVersionByte = 6;
    static constexpr unsigned kVersionShift = 4;

    Bytes m_bytes{};
};

} // namespace phvikapen::core
