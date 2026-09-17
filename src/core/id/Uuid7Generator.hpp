#pragma once

#include "core/id/Uuid.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

namespace phvikapen::core {

class Uuid7Generator {
public:
    using UnixClock = std::function<std::chrono::milliseconds()>;
    using RandomSource = std::function<std::uint64_t()>;

    Uuid7Generator();

    Uuid7Generator(UnixClock clock, RandomSource random);

    [[nodiscard]] Uuid next();

private:
    [[nodiscard]] std::uint16_t freshCounter();

    UnixClock m_clock;
    RandomSource m_random;
    std::optional<std::uint64_t> m_lastTimestampMs;
    std::uint16_t m_counter{};
};

}
