#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/undo/UndoStack.hpp"

#include <vector>

namespace phvikapen::core {

class NotebookStore;

class AddStrokeCommand final : public ICommand {
public:
    AddStrokeCommand(NotebookStore* store, const Uuid& pageId, PlacedStroke placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    NotebookStore* m_store;
    Uuid m_pageId;
    PlacedStroke m_placed;
};

class ClearPageCommand final : public ICommand {
public:
    ClearPageCommand(NotebookStore* store, const Uuid& pageId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    NotebookStore* m_store;
    Uuid m_pageId;
    std::vector<PlacedStroke> m_removed;
};

}
