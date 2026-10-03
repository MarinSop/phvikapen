#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"

#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {

[[nodiscard]] bool inside(std::span<const Point> polygon, Point point) noexcept;

[[nodiscard]] std::vector<Uuid> strokesInside(const Page& page, std::span<const Point> polygon);

// The stroke under a point, within `reach` of it: the last of those it could be, which is the one
// lying over the others. A loop takes what is wholly inside it; a touch takes what is under it.
[[nodiscard]] std::optional<Uuid> strokeUnder(const Page& page, Point at, float reach);

[[nodiscard]] std::optional<Rect> boundsOf(const Page& page, std::span<const Uuid> strokeIds);

[[nodiscard]] Stroke moved(const Stroke& stroke, float dx, float dy);

}
