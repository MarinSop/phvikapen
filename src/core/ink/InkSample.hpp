#pragma once

#include <chrono>

namespace phvikapen::core {

struct InkSample {
    float x{};
    float y{};
    float pressure{1.0F};
    float tiltX{};
    float tiltY{};
    std::chrono::microseconds timestamp{};

    friend bool operator==(const InkSample&, const InkSample&) = default;
};

}
