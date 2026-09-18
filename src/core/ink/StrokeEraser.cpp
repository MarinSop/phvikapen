#include "core/ink/StrokeEraser.hpp"

#include "core/geometry/Distance.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr std::size_t kSmallestPiece = 2;

[[nodiscard]] bool rubbedOut(const InkSample& sample,
                             std::span<const EraserSweep> sweeps) noexcept {
    const Point point{.x = sample.x, .y = sample.y};
    return std::ranges::any_of(sweeps, [&point](const EraserSweep& sweep) {
        return squaredDistanceToSegment(point, sweep.from, sweep.to) <= sweep.radius * sweep.radius;
    });
}

}

std::vector<Stroke> erased(const Stroke& stroke, std::span<const EraserSweep> sweeps,
                           Uuid7Generator& ids) {
    std::vector<Stroke> pieces;
    std::vector<InkSample> kept;

    const auto finish = [&] {
        if (kept.size() < kSmallestPiece) {
            kept.clear();
            return;
        }
        Stroke piece{ids.next(), stroke.style()};
        for (const InkSample& sample : kept) {
            piece.append(sample);
        }
        pieces.push_back(std::move(piece));
        kept.clear();
    };

    for (const InkSample& sample : stroke.samples()) {
        if (rubbedOut(sample, sweeps)) {
            finish();
            continue;
        }
        kept.push_back(sample);
    }
    finish();
    return pieces;
}

bool wholeStrokeSurvives(const Stroke& stroke, std::span<const Stroke> pieces) noexcept {
    return pieces.size() == 1 && pieces.front().samples().size() == stroke.samples().size();
}

}
