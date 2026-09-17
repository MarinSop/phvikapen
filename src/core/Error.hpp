#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace phvikapen::core {

enum class ErrorCode : std::uint8_t {
    Unknown,
    InvalidArgument,
    NotFound,
    IoFailure,
    Unsupported,
};

[[nodiscard]] std::string_view toString(ErrorCode code) noexcept;

struct Error {
    ErrorCode code{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] friend bool operator==(const Error&, const Error&) = default;
};

template <typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> makeError(ErrorCode code, std::string message) {
    return std::unexpected<Error>{Error{.code = code, .message = std::move(message)}};
}

}
