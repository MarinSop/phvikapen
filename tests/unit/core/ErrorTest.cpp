#include "core/Error.hpp"

#include <gtest/gtest.h>

namespace phvikapen::core {
namespace {

[[nodiscard]] Result<int> requirePositive(int value) {
    if (value <= 0) {
        return makeError(ErrorCode::InvalidArgument, "value must be positive");
    }
    return value;
}

TEST(ResultTest, HoldsValueOnSuccess) {
    const auto result = requirePositive(3);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 3);
}

TEST(ResultTest, HoldsErrorOnFailure) {
    const auto result = requirePositive(-1);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(),
              (Error{.code = ErrorCode::InvalidArgument, .message = "value must be positive"}));
}

TEST(ErrorCodeTest, HasStableIdentifiers) {
    EXPECT_EQ(toString(ErrorCode::Unknown), "unknown");
    EXPECT_EQ(toString(ErrorCode::InvalidArgument), "invalid_argument");
    EXPECT_EQ(toString(ErrorCode::NotFound), "not_found");
    EXPECT_EQ(toString(ErrorCode::IoFailure), "io_failure");
    EXPECT_EQ(toString(ErrorCode::Unsupported), "unsupported");
}

} // namespace
} // namespace phvikapen::core
