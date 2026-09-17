#pragma once

#include "core/Error.hpp"
#include "core/model/Page.hpp"
#include "core/undo/UndoStack.hpp"

#include <vector>

namespace phvikapen::core {

class StorageThread;

class AddStrokeCommand final : public ICommand {
public:
    AddStrokeCommand(Page* page, StorageThread* storage, PlacedStroke placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    Page* m_page;
    StorageThread* m_storage;
    PlacedStroke m_placed;
};

class ClearPageCommand final : public ICommand {
public:
    ClearPageCommand(Page* page, StorageThread* storage) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<PlacedStroke> m_removed;
};

}
