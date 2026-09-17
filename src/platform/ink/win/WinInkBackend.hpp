#pragma once

// TODO(M1): Implement after the ink spike (ADR 0002).

#include "platform/ink/IInkBackend.hpp"

namespace phvikapen::platform::ink {

class WinInkBackend final : public IInkBackend {
public:
    [[nodiscard]] std::string_view name() const noexcept override;

    void setSink(IInkSink* sink) noexcept override;

    void setStrokeStyle(const core::StrokeStyle& style) override;

private:
    IInkSink* m_sink{nullptr};
    core::StrokeStyle m_style;
};

}
