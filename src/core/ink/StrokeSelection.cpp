#include "core/ink/StrokeSelection.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] std::optional<Rect> boundsOfPolygon(std::span<const Point> polygon) noexcept {
    if (polygon.empty()) {
        return std::nullopt;
    }
    Rect bounds{
        .left = polygon.front().x,
        .top = polygon.front().y,
        .right = polygon.front().x,
        .bottom = polygon.front().y,
    };
    for (const Point& corner : polygon) {
        bounds.left = std::min(bounds.left, corner.x);
        bounds.top = std::min(bounds.top, corner.y);
        bounds.right = std::max(bounds.right, corner.x);
        bounds.bottom = std::max(bounds.bottom, corner.y);
    }
    return bounds;
}

[[nodiscard]] bool holds(const Rect& outer, const Rect& inner) noexcept {
    return inner.left >= outer.left && inner.right <= outer.right && inner.top >= outer.top
           && inner.bottom <= outer.bottom;
}

}

bool inside(std::span<const Point> polygon, Point point) noexcept {
    if (polygon.size() < 3) {
        return false;
    }
    bool within = false;
    for (std::size_t i = 0, previous = polygon.size() - 1; i < polygon.size(); previous = i++) {
        const Point& corner = polygon[i];
        const Point& before = polygon[previous];
        const bool straddles = (corner.y > point.y) != (before.y > point.y);
        if (!straddles) {
            continue;
        }
        const float crossing =
            corner.x + ((point.y - corner.y) / (before.y - corner.y) * (before.x - corner.x));
        if (point.x < crossing) {
            within = !within;
        }
    }
    return within;
}

std::vector<Uuid> strokesInside(const Page& page, std::span<const Point> polygon) {
    std::vector<Uuid> found;
    const std::optional<Rect> around = boundsOfPolygon(polygon);
    if (!around || polygon.size() < 3) {
        return found;
    }

    for (const PlacedStroke& placed : page.strokes()) {
        const std::optional<Rect> bounds = placed.stroke.boundingBox();
        if (!bounds || !holds(*around, *bounds)) {
            continue;
        }
        const bool whollyInside =
            std::ranges::all_of(placed.stroke.samples(), [&polygon](const InkSample& sample) {
                return inside(polygon, Point{.x = sample.x, .y = sample.y});
            });
        if (whollyInside) {
            found.push_back(placed.stroke.id());
        }
    }
    return found;
}

std::optional<Rect> boundsOf(const Page& page, std::span<const Uuid> strokeIds) {
    std::optional<Rect> bounds;
    for (const PlacedStroke& placed : page.strokes()) {
        if (std::ranges::find(strokeIds, placed.stroke.id()) == strokeIds.end()) {
            continue;
        }
        if (const std::optional<Rect> box = placed.stroke.boundingBox()) {
            bounds = bounds ? bounds->united(*box) : *box;
        }
    }
    return bounds;
}

Stroke moved(const Stroke& stroke, float dx, float dy) {
    Stroke copy{stroke.id(), stroke.style()};
    for (const InkSample& sample : stroke.samples()) {
        InkSample shifted = sample;
        shifted.x += dx;
        shifted.y += dy;
        copy.append(shifted);
    }
    return copy;
}

}
