#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/undo/UndoStack.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace phvikapen::core {

// Several changes that belong together: they go in as one step and come out as one step, so a
// reader who undoes gets back exactly what they had.
class BundleCommand final : public ICommand {
public:
    explicit BundleCommand(std::vector<std::unique_ptr<ICommand>> steps) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    std::vector<std::unique_ptr<ICommand>> m_steps;
};

}
