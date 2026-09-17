#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/model/PageStyle.hpp"

#include <optional>

namespace phvikapen::core {

struct ViewSize {
    float width{};
    float height{};

    friend constexpr bool operator==(const ViewSize&, const ViewSize&) = default;
};

class Viewport {
public:
    static constexpr float kMinimumScale = 0.25F;
    static constexpr float kMaximumScale = 8.0F;
    static constexpr float kPaperMargin = 24.0F;

    [[nodiscard]] Point origin() const noexcept { return m_origin; }

    [[nodiscard]] float scale() const noexcept { return m_scale; }

    [[nodiscard]] Point toPage(Point view) const noexcept;
    [[nodiscard]] Point toView(Point page) const noexcept;
    [[nodiscard]] Rect visiblePage(ViewSize view) const noexcept;

    void panBy(float viewDeltaX, float viewDeltaY) noexcept;
    void zoomAround(Point view, float factor) noexcept;
    void setScale(float scale) noexcept;

    void fit(ViewSize view, std::optional<PaperSize> paper) noexcept;
    void keepPaperInView(ViewSize view, std::optional<PaperSize> paper) noexcept;

    friend bool operator==(const Viewport&, const Viewport&) = default;

private:
    Point m_origin;
    float m_scale{1.0F};
};

}
