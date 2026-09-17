#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/undo/UndoStack.hpp"

#include <vector>

namespace phvikapen::core {

class NotebookStore;

/// Puts one stroke on a page, and takes it off again.
///
/// The command keeps the stroke itself, not a reference to it, so that it can put the stroke back
/// long after the page stopped holding it. The store must outlive the command.
class AddStrokeCommand final : public ICommand {
public:
    AddStrokeCommand(NotebookStore* store, const Uuid& pageId, Stroke stroke) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    NotebookStore* m_store;
    Uuid m_pageId;
    Stroke m_stroke;
};

/// Takes everything off a page, and puts it back in the order it was drawn.
///
/// Emptying a page is one action to the user, so it is one command: a single undo brings the whole
/// page back. The strokes are held by the command while it is in the history, which is what makes
/// the undo possible after the rows are gone. The store must outlive the command.
class ClearPageCommand final : public ICommand {
public:
    ClearPageCommand(NotebookStore* store, const Uuid& pageId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    NotebookStore* m_store;
    Uuid m_pageId;
    std::vector<Stroke> m_removed;
};

} // namespace phvikapen::core
