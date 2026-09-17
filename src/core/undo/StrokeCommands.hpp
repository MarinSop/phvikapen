#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/undo/UndoStack.hpp"

#include <vector>

namespace phvikapen::core {

class NotebookStore;

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

}
