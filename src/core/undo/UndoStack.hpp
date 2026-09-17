#pragma once

#include "core/Error.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace phvikapen::core {

/// One reversible change to a notebook.
///
/// A command owns everything it needs to take its change back, so that the history survives even
/// when what it changed is no longer on screen. Several small edits that the user sees as one
/// action, such as a single sweep of the eraser, belong in one command.
class ICommand {
public:
    ICommand() = default;
    virtual ~ICommand() = default;

    ICommand(const ICommand&) = delete;
    ICommand& operator=(const ICommand&) = delete;
    ICommand(ICommand&&) = delete;
    ICommand& operator=(ICommand&&) = delete;

    /// Performs the change: once when the command is run, and again for every redo.
    [[nodiscard]] virtual Result<void> apply() = 0;

    /// Takes the change back, restoring what apply() found.
    [[nodiscard]] virtual Result<void> revert() = 0;
};

/// Undo history of one notebook.
///
/// Commands are kept in the order they were applied. Undoing walks backwards through them without
/// dropping anything, so redo can walk forward again; running a new command after an undo is what
/// drops the commands that were undone, because they belong to a history that no longer happened.
///
/// A command that fails leaves the history exactly as it was, so that what the history says and
/// what the notebook holds never disagree.
class UndoStack {
public:
    /// How many commands are remembered before the oldest is forgotten. Deep enough to cover a
    /// writing session, small enough that the strokes a command holds cannot grow without bound.
    static constexpr std::size_t kDefaultDepthLimit = 200;

    /// Creates a history that remembers at most @p depthLimit commands, and never fewer than one.
    explicit UndoStack(std::size_t depthLimit = kDefaultDepthLimit) noexcept;

    /// Applies @p command and, when that succeeds, makes it the command undo() takes back.
    [[nodiscard]] Result<void> run(std::unique_ptr<ICommand> command);

    [[nodiscard]] bool canUndo() const noexcept { return m_applied != 0; }

    [[nodiscard]] bool canRedo() const noexcept { return m_applied < m_commands.size(); }

    /// Takes the most recent command back. Fails with ErrorCode::NotFound when there is none.
    [[nodiscard]] Result<void> undo();

    /// Applies the command that was undone last. Fails with ErrorCode::NotFound when there is none.
    [[nodiscard]] Result<void> redo();

    /// Forgets the history, for example when another notebook is opened. What was done stays done.
    void clear() noexcept;

    [[nodiscard]] std::size_t undoCount() const noexcept { return m_applied; }

    [[nodiscard]] std::size_t redoCount() const noexcept { return m_commands.size() - m_applied; }

private:
    std::vector<std::unique_ptr<ICommand>> m_commands;
    /// Commands before this index are applied; the ones from it on were undone.
    std::size_t m_applied{0};
    std::size_t m_depthLimit;
};

} // namespace phvikapen::core
