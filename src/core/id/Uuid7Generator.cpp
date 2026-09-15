#include "core/id/Uuid7Generator.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ranges>
#include <span>
#include <utility>

namespace phvikapen::core {
namespace {

constexpr std::uint64_t kTimestampMask = (std::uint64_t{1} << 48U) - 1U;
constexpr std::uint16_t kMaxCounter = 0x0FFF;
// A fresh counter starts with its top bit cleared, leaving room to count up (RFC 9562 section 6.2).
constexpr std::uint64_t kCounterSeedMask = 0x07FF;
constexpr std::uint64_t kVersionBits = 0x7000;
constexpr std::uint64_t kVariantBits = 0x8000'0000'0000'0000;
constexpr std::uint64_t kRandomMask = 0x3FFF'FFFF'FFFF'FFFF;
constexpr std::uint64_t kByteMask = 0xFF;
constexpr unsigned kBitsPerByte = 8;
constexpr std::size_t kTimestampBytes = 6;
constexpr std::size_t kVersionAndCounterBytes = 2;

/// Writes the low out.size() bytes of @p value into @p out in big-endian order.
constexpr void putBigEndian(std::span<std::uint8_t> out, std::uint64_t value) noexcept {
    for (std::uint8_t& byte : std::views::reverse(out)) {
        byte = static_cast<std::uint8_t>(value & kByteMask);
        value >>= kBitsPerByte;
    }
}

std::chrono::milliseconds systemUnixTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

Uuid7Generator::RandomSource makeDefaultRandomSource() {
    std::random_device device;
    std::seed_seq seed{device(), device(), device(), device()};
    return [engine = std::mt19937_64{seed}] mutable { return engine(); };
}

} // namespace

Uuid7Generator::Uuid7Generator() : Uuid7Generator(systemUnixTime, makeDefaultRandomSource()) {}

Uuid7Generator::Uuid7Generator(UnixClock clock, RandomSource random)
    : m_clock{std::move(clock)}, m_random{std::move(random)} {}

Uuid Uuid7Generator::next() {
    auto timestampMs = static_cast<std::uint64_t>(m_clock().count()) & kTimestampMask;

    if (m_lastTimestampMs && timestampMs <= *m_lastTimestampMs) {
        // Same millisecond, or the clock stepped back: keep the last timestamp and count up.
        timestampMs = *m_lastTimestampMs;
        if (m_counter < kMaxCounter) {
            ++m_counter;
        } else {
            // Counter exhausted: move on to the next millisecond (RFC 9562 section 6.2).
            timestampMs = (timestampMs + 1U) & kTimestampMask;
            m_counter = freshCounter();
        }
    } else {
        m_counter = freshCounter();
    }
    m_lastTimestampMs = timestampMs;

    Uuid::Bytes bytes{};
    const std::span<std::uint8_t> out{bytes};
    putBigEndian(out.first(kTimestampBytes), timestampMs);
    putBigEndian(out.subspan(kTimestampBytes, kVersionAndCounterBytes), kVersionBits | m_counter);
    putBigEndian(out.subspan(kTimestampBytes + kVersionAndCounterBytes),
                 kVariantBits | (m_random() & kRandomMask));
    return Uuid{bytes};
}

std::uint16_t Uuid7Generator::freshCounter() {
    return static_cast<std::uint16_t>(m_random() & kCounterSeedMask);
}

} // namespace phvikapen::core
