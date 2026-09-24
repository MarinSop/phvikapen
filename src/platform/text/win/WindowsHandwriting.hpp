#pragma once

#include "core/Error.hpp"
#include "core/ink/Stroke.hpp"
#include "core/text/InkWord.hpp"
#include "platform/text/IHandwriting.hpp"

#include <span>
#include <string>
#include <vector>

namespace phvikapen::platform::text {

// Reads handwriting with the reader Windows carries. One of these belongs to the thread that made
// it, and blocks that thread while it reads.
class WindowsHandwriting final : public IHandwriting {
public:
    WindowsHandwriting();
    ~WindowsHandwriting() override;

    WindowsHandwriting(const WindowsHandwriting&) = delete;
    WindowsHandwriting& operator=(const WindowsHandwriting&) = delete;
    WindowsHandwriting(WindowsHandwriting&&) = delete;
    WindowsHandwriting& operator=(WindowsHandwriting&&) = delete;

    [[nodiscard]] core::Result<std::vector<core::InkWord>>
    read(std::span<const core::Stroke> strokes) override;

    [[nodiscard]] std::vector<std::string> languages() override;

private:
    bool m_apartment{false};
};

}
