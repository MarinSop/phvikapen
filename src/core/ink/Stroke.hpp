#pragma once

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {

/// 8-bit per channel color with straight (non-premultiplied) alpha.
struct Color {
    static constexpr std::uint8_t kOpaque = 255;

    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{kOpaque};

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

/// Appearance of a stroke.
struct StrokeStyle {
    static constexpr float kDefaultWidth = 2.0F;

    Color color{};
    /// Nominal width in page units at full pressure.
    float width{kDefaultWidth};

    friend constexpr bool operator==(const StrokeStyle&, const StrokeStyle&) = default;
};

/// A single continuous pen stroke: its samples in input order and the style it is drawn with.
class Stroke {
public:
    explicit Stroke(Uuid id, StrokeStyle style = {}) noexcept;

    [[nodiscard]] const Uuid& id() const noexcept { return m_id; }

    [[nodiscard]] const StrokeStyle& style() const noexcept { return m_style; }

    [[nodiscard]] std::span<const InkSample> samples() const noexcept { return m_samples; }

    [[nodiscard]] bool empty() const noexcept { return m_samples.empty(); }

    /// Appends a sample and extends the bounds. Amortized O(1).
    void append(const InkSample& sample);

    /// Bounds of the rendered stroke: the sample positions grown by half the nominal width.
    /// Returns std::nullopt for a stroke without samples.
    [[nodiscard]] std::optional<Rect> boundingBox() const noexcept;

private:
    Uuid m_id;
    StrokeStyle m_style;
    std::vector<InkSample> m_samples;
    std::optional<Rect> m_sampleBounds;
};

} // namespace phvikapen::core
