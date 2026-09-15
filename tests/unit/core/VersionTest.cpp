#include "core/version.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>

namespace phvikapen::core {
namespace {

TEST(VersionTest, StringMatchesComponents) {
    const std::string expected =
        std::format("{}.{}.{}", version::kMajor, version::kMinor, version::kPatch);

    EXPECT_EQ(version::kString, expected);
}

} // namespace
} // namespace phvikapen::core
