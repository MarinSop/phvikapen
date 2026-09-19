#pragma once

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/model/Color.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {

struct StrokeStyle {
    static constexpr float kDefaultWidth = 2.0F;

    Color color{};
    float width{kDefaultWidth};
    // A pen leaves round ends; a drawn shape may be asked for square ones instead.
    bool roundEnds{true};

    friend constexpr bool operator==(const StrokeStyle&, const StrokeStyle&) = default;
};

// A light touch still leaves a line, and the pressures a writing hand uses spread over the width.
[[nodiscard]] float widthAt(const StrokeStyle& style, float pressure) noexcept;

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
