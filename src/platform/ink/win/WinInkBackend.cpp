#include "platform/ink/win/WinInkBackend.hpp"

namespace phvikapen::platform::ink {

std::string_view WinInkBackend::name() const noexcept {
    return "windows";
}

void WinInkBackend::setSink(IInkSink* sink) noexcept {
    m_sink = sink;
}

void WinInkBackend::setStrokeStyle(const core::StrokeStyle& style) {
    m_style = style;
}

} // namespace phvikapen::platform::ink
