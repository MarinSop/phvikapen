#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/Table.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>

namespace phvikapen::core {

class StorageThread;

class AddTableCommand final : public ICommand {
public:
    AddTableCommand(Page* page, StorageThread* storage, PlacedTable placed);

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    PlacedTable m_placed;
};

class RemoveTableCommand final : public ICommand {
public:
    RemoveTableCommand(Page* page, StorageThread* storage, const Uuid& tableId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    Uuid m_tableId;
    std::optional<PlacedTable> m_removed;
};

// Everything about a table that can change while it keeps its place in the order of the page:
// where it stands, how wide its columns run, how tall its rows stand, what is typed in its boxes
// and the face all of it wears.
class ChangeTableCommand final : public ICommand {
public:
    ChangeTableCommand(Page* page, StorageThread* storage, Table table);

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> write(Table table);

    Page* m_page;
    StorageThread* m_storage;
    Table m_after;
    std::optional<Table> m_before;
};

}
