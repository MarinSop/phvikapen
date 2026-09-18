#pragma once

#include "core/geometry/Distance.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <span>
#include <string_view>

namespace phvikapen::platform::ink {

class IInkSink {
public:
    virtual ~IInkSink() = default;

    virtual void strokeStarted(const core::InkSample& sample) = 0;

    virtual void sampleAdded(const core::InkSample& sample) = 0;

    virtual void strokeFinished(const core::InkSample& sample) = 0;

    // Filtered stroke; the other calls carry raw device samples.
    virtual void strokeCompleted(const core::Stroke& stroke) = 0;

    virtual void strokeCancelled() = 0;

    virtual void eraserMoved(const core::InkSample& from, const core::InkSample& to,
                             float radius) = 0;

    virtual void eraseFinished() = 0;

    virtual void selectionDrawn(std::span<const core::Point> shape) = 0;

    virtual void selectionMoved(float dx, float dy) = 0;
};

class IInkBackend {
public:
    virtual ~IInkBackend() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual void setSink(IInkSink* sink) noexcept = 0;

    virtual void setStrokeStyle(const core::StrokeStyle& style) = 0;

    virtual void setErasing(bool erasing) = 0;

    virtual void setSelecting(bool selecting) = 0;
};

}
