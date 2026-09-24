#pragma once

#include "core/Error.hpp"
#include "core/ink/Stroke.hpp"
#include "core/text/InkWord.hpp"

#include <memory>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::platform::text {

class IHandwriting {
public:
    virtual ~IHandwriting() = default;

    IHandwriting() = default;
    IHandwriting(const IHandwriting&) = delete;
    IHandwriting& operator=(const IHandwriting&) = delete;
    IHandwriting(IHandwriting&&) = delete;
    IHandwriting& operator=(IHandwriting&&) = delete;

    // The words the strokes were written in, in the order a reader would read them.
    [[nodiscard]] virtual core::Result<std::vector<core::InkWord>>
    read(std::span<const core::Stroke> strokes) = 0;

    // The languages this machine can read handwriting in.
    [[nodiscard]] virtual std::vector<std::string> languages() = 0;
};

// A reader for this machine, or nothing where handwriting cannot be read. Use it from one thread.
[[nodiscard]] std::unique_ptr<IHandwriting> openHandwriting();

}
