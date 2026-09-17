#include "core/Error.hpp"

#include <string_view>

namespace phvikapen::core {

std::string_view toString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::Unknown:
        return "unknown";
    case ErrorCode::InvalidArgument:
        return "invalid_argument";
    case ErrorCode::NotFound:
        return "not_found";
    case ErrorCode::IoFailure:
        return "io_failure";
    case ErrorCode::Unsupported:
        return "unsupported";
    }
    return "unknown";
}

}
