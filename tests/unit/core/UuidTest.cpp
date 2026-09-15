#include "core/id/Uuid.hpp"

#include "core/id/Uuid7Generator.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <regex>
#include <vector>

namespace phvikapen::core {
namespace {

using std::chrono::milliseconds;

[[nodiscard]] Uuid7Generator::UnixClock fixedClock(milliseconds now) {
    return [now] { return now; };
}

/// Deterministic random source returning 0, 1, 2, ...
[[nodiscard]] Uuid7Generator::RandomSource sequentialRandom() {
    return [value = std::uint64_t{0}] mutable { return value++; };
}

TEST(UuidTest, DefaultConstructedIsNil) {
    const Uuid uuid;

    EXPECT_TRUE(uuid.isNil());
    EXPECT_EQ(uuid.toString(), "00000000-0000-0000-0000-000000000000");
}

TEST(Uuid7GeneratorTest, ProducesCanonicalVersion7Strings) {
    Uuid7Generator generator;
    const std::regex canonical{
        "^[0-9a-f]{8}-[0-9a-f]{4}-7[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"};

    for (int i = 0; i < 100; ++i) {
        const Uuid uuid = generator.next();
        EXPECT_EQ(uuid.version(), 7);
        EXPECT_TRUE(std::regex_match(uuid.toString(), canonical)) << uuid.toString();
    }
}

TEST(Uuid7GeneratorTest, EncodesUnixTimestampInFirst48Bits) {
    Uuid7Generator generator{fixedClock(milliseconds{0x0123'4567'89AB}), sequentialRandom()};

    EXPECT_TRUE(generator.next().toString().starts_with("01234567-89ab-7"));
}

TEST(Uuid7GeneratorTest, IsStrictlyIncreasingWithinOneMillisecond) {
    // More values than the 12-bit counter can hold, so counter exhaustion is exercised too.
    Uuid7Generator generator{fixedClock(milliseconds{1'700'000'000'000}), sequentialRandom()};

    Uuid previous = generator.next();
    for (int i = 0; i < 10'000; ++i) {
        const Uuid current = generator.next();
        ASSERT_LT(previous, current)
            << "index " << i << ": " << previous.toString() << " >= " << current.toString();
        previous = current;
    }
}

TEST(Uuid7GeneratorTest, StaysIncreasingWhenClockStepsBack) {
    const std::vector<milliseconds> ticks{
        milliseconds{5'000},
        milliseconds{4'000},
        milliseconds{4'500},
    };
    Uuid7Generator generator{[&ticks, index = std::size_t{0}] mutable { return ticks.at(index++); },
                             sequentialRandom()};

    const Uuid first = generator.next();
    const Uuid second = generator.next();
    const Uuid third = generator.next();

    EXPECT_LT(first, second);
    EXPECT_LT(second, third);
}

} // namespace
} // namespace phvikapen::core
