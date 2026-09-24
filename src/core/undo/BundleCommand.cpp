#include "core/undo/BundleCommand.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace phvikapen::core {

BundleCommand::BundleCommand(std::vector<std::unique_ptr<ICommand>> steps) noexcept
    : m_steps{std::move(steps)} {}

Result<void> BundleCommand::apply() {
    for (std::size_t done = 0; done < m_steps.size(); ++done) {
        if (const Result<void> applied = m_steps[done]->apply(); !applied) {
            // A step that will not go in leaves the ones before it undone, so nothing is half done.
            for (std::size_t back = done; back > 0; --back) {
                std::ignore = m_steps[back - 1]->revert();
            }
            return applied;
        }
    }
    return {};
}

Result<void> BundleCommand::revert() {
    for (std::size_t left = m_steps.size(); left > 0; --left) {
        if (const Result<void> reverted = m_steps[left - 1]->revert(); !reverted) {
            for (std::size_t again = left; again < m_steps.size(); ++again) {
                std::ignore = m_steps[again]->apply();
            }
            return reverted;
        }
    }
    return {};
}

std::optional<Uuid> BundleCommand::pageToShow() const {
    for (const std::unique_ptr<ICommand>& step : m_steps) {
        if (const std::optional<Uuid> page = step->pageToShow()) {
            return page;
        }
    }
    return std::nullopt;
}

}
