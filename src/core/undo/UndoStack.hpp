#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace phvikapen::core {

class ICommand {
public:
    ICommand() = default;
    virtual ~ICommand() = default;

    ICommand(const ICommand&) = delete;
    ICommand& operator=(const ICommand&) = delete;
    ICommand(ICommand&&) = delete;
    ICommand& operator=(ICommand&&) = delete;

    [[nodiscard]] virtual Result<void> apply() = 0;

    [[nodiscard]] virtual Result<void> revert() = 0;

    [[nodiscard]] virtual std::optional<Uuid> pageToShow() const { return std::nullopt; }
};

class UndoStack {
public:
    static constexpr std::size_t kDefaultDepthLimit = 200;

    explicit UndoStack(std::size_t depthLimit = kDefaultDepthLimit) noexcept;

    [[nodiscard]] Result<void> run(std::unique_ptr<ICommand> command);

    [[nodiscard]] bool canUndo() const noexcept { return m_applied != 0; }

    [[nodiscard]] bool canRedo() const noexcept { return m_applied < m_commands.size(); }

    [[nodiscard]] Result<void> undo();

    [[nodiscard]] Result<void> redo();

    void clear() noexcept;

    [[nodiscard]] const ICommand* nextUndo() const noexcept;

    [[nodiscard]] const ICommand* nextRedo() const noexcept;

    [[nodiscard]] std::size_t undoCount() const noexcept { return m_applied; }

    [[nodiscard]] std::size_t redoCount() const noexcept { return m_commands.size() - m_applied; }

private:
    std::vector<std::unique_ptr<ICommand>> m_commands;
    std::size_t m_applied{0};
    std::size_t m_depthLimit;
};

}
