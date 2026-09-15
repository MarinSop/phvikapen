#pragma once

// TODO(M1): Native low-latency ink backend for Windows, chosen after a measured spike.
//
// Option A: CoreInkPresenterHost (C++/WinRT)
//   + System Windows Ink rendering on its own thread, lowest effort for low latency.
//   - Needs Windows.UI.Composition interop with the Qt window; less control over filtering.
//
// Option B: Direct2D + DirectComposition (C++/WinRT)
//   Flip-model waitable swap chain with a maximum frame latency of 1, fed by WM_POINTER input.
//   + Full control over filtering, prediction and rendering.
//   - More code; prediction is our responsibility.
//
// Both prototypes are measured on the target device (pen-to-pixel latency with a 240 fps
// camera) before the decision is recorded in docs/adr/0002-ink-backend.md.

#include "platform/ink/IInkBackend.hpp"

namespace phvikapen::platform::ink {

/// Placeholder for the native Windows backend. Not functional yet.
class WinInkBackend final : public IInkBackend {
public:
    [[nodiscard]] std::string_view name() const noexcept override;

    void setSink(IInkSink* sink) noexcept override;

    void setStrokeStyle(const core::StrokeStyle& style) override;

private:
    IInkSink* m_sink{nullptr};
    core::StrokeStyle m_style;
};

} // namespace phvikapen::platform::ink
