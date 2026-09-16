#pragma once

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <string_view>

namespace phvikapen::platform::ink {

/// Receives the raw samples of strokes captured by an ink backend.
///
/// Every stroke is reported as strokeStarted(), zero or more sampleAdded() calls, and then
/// either strokeFinished() or strokeCancelled(). Backends document the thread they call from;
/// implementations must not block, because they sit on the latency-critical input path.
class IInkSink {
public:
    virtual ~IInkSink() = default;

    /// The pen touched down. @p sample is the first sample of the new stroke.
    virtual void strokeStarted(const core::InkSample& sample) = 0;

    /// The pen moved while in contact.
    virtual void sampleAdded(const core::InkSample& sample) = 0;

    /// The pen lifted. @p sample is the last sample of the stroke.
    virtual void strokeFinished(const core::InkSample& sample) = 0;

    /// The finished stroke, as it should be drawn and stored. Unlike the calls above, which carry
    /// the raw samples of the device, this one carries the stroke after filtering.
    virtual void strokeCompleted(const core::Stroke& stroke) = 0;

    /// The stroke was interrupted, for example because the input device was lost.
    virtual void strokeCancelled() = 0;
};

/// Captures pen input and renders wet ink (the stroke being written) with minimal latency.
///
/// Implementations:
/// - Qt fallback: pointer and tablet events on a Qt Quick item, rendered through QRhi.
///   Used on macOS for development and as a fallback on Windows.
/// - Windows native: low-latency ink on its own input thread (TODO(M1), see ink/win).
///
/// A backend does not own the finished strokes. It reports samples to its sink, and the
/// application moves finished strokes into the document model (dry ink).
class IInkBackend {
public:
    virtual ~IInkBackend() = default;

    /// Short identifier for logs, for example "qt" or "windows".
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// Sets the receiver of stroke events. The sink is not owned and may be null.
    virtual void setSink(IInkSink* sink) noexcept = 0;

    /// Style used to render wet ink for strokes started after this call.
    virtual void setStrokeStyle(const core::StrokeStyle& style) = 0;
};

} // namespace phvikapen::platform::ink
