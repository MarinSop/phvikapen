#pragma once

#include "core/id/Uuid.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

namespace phvikapen::core {

/// Generates time-ordered UUIDv7 values (RFC 9562 section 5.7).
///
/// Layout: 48-bit Unix timestamp in milliseconds, 4-bit version, 12-bit counter
/// (RFC 9562 section 6.2, method 1), 2-bit variant and 62 random bits.
/// Values from one generator are strictly increasing, even when several are created
/// within the same millisecond or when the system clock steps backwards.
///
/// The default random source is a pseudo-random engine seeded from std::random_device.
/// Identifiers are unique but not meant to be unguessable.
///
/// Not thread-safe: use one generator per thread or synchronize access externally.
class Uuid7Generator {
public:
    /// Returns the current Unix time in milliseconds.
    using UnixClock = std::function<std::chrono::milliseconds()>;
    /// Returns uniformly distributed random bits.
    using RandomSource = std::function<std::uint64_t()>;

    /// Uses the system clock and a randomly seeded pseudo-random engine.
    Uuid7Generator();

    /// Uses the given clock and random source, for example to make tests deterministic.
    Uuid7Generator(UnixClock clock, RandomSource random);

    [[nodiscard]] Uuid next();

private:
    [[nodiscard]] std::uint16_t freshCounter();

    UnixClock m_clock;
    RandomSource m_random;
    std::optional<std::uint64_t> m_lastTimestampMs;
    std::uint16_t m_counter{};
};

} // namespace phvikapen::core
