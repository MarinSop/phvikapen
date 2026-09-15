#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace phvikapen::core {

/// Category of a failure that crosses a layer boundary.
enum class ErrorCode : std::uint8_t {
    Unknown,
    InvalidArgument,
    NotFound,
    IoFailure,
    Unsupported,
};

/// Returns a stable, lowercase identifier for @p code, suitable for logs.
[[nodiscard]] std::string_view toString(ErrorCode code) noexcept;

/// Error value carried by Result instead of exceptions.
struct Error {
    ErrorCode code{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] friend bool operator==(const Error&, const Error&) = default;
};

/// Outcome of an operation that can fail. Errors cross layer boundaries as values.
template <typename T>
using Result = std::expected<T, Error>;

/// Creates the error branch of a Result.
[[nodiscard]] inline std::unexpected<Error> makeError(ErrorCode code, std::string message) {
    return std::unexpected<Error>{Error{.code = code, .message = std::move(message)}};
}

} // namespace phvikapen::core
