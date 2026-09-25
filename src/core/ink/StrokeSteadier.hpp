#pragma once

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {

// Averages each sample with its neighbours along the line, never over time, and keeps both ends.
// The averaging stops at a corner, so that a line can be evened out far enough to lose the shake
// of a slow hand without the corners of what is written being rounded away with it.
class StrokeSteadier {
public:
    explicit StrokeSteadier(float reach) noexcept;

    void append(const InkSample& sample);

    [[nodiscard]] std::span<const InkSample> settled();

    [[nodiscard]] float reach() const noexcept { return m_reach; }

private:
    void findTurns();

    void steady(std::size_t index);

    [[nodiscard]] float roomBefore(std::size_t index) const noexcept;

    [[nodiscard]] float roomAfter(std::size_t index) const noexcept;

    float m_reach;
    std::vector<InkSample> m_drawn;
    std::vector<float> m_along;
    std::vector<float> m_turn;
    std::vector<bool> m_corner;
    std::vector<InkSample> m_steadied;
    std::size_t m_unsettled{0};
};

[[nodiscard]] std::vector<InkSample> steadied(std::span<const InkSample> samples, float reach);

[[nodiscard]] Stroke steadied(const Stroke& stroke, float reach);

}
