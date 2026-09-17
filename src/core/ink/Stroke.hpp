#pragma once

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {

struct Color {
    static constexpr std::uint8_t kOpaque = 255;

    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{kOpaque};

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

struct StrokeStyle {
    static constexpr float kDefaultWidth = 2.0F;

    Color color{};
    float width{kDefaultWidth};

    friend constexpr bool operator==(const StrokeStyle&, const StrokeStyle&) = default;
};

class Stroke {
public:
    explicit Stroke(Uuid id, StrokeStyle style = {}) noexcept;

    [[nodiscard]] const Uuid& id() const noexcept { return m_id; }

    [[nodiscard]] const StrokeStyle& style() const noexcept { return m_style; }

    [[nodiscard]] std::span<const InkSample> samples() const noexcept { return m_samples; }

    [[nodiscard]] bool empty() const noexcept { return m_samples.empty(); }

    void append(const InkSample& sample);

    [[nodiscard]] std::optional<Rect> boundingBox() const noexcept;

private:
    Uuid m_id;
    StrokeStyle m_style;
    std::vector<InkSample> m_samples;
    std::optional<Rect> m_sampleBounds;
};

}
