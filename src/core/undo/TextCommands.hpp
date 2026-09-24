#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/TextBox.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>

namespace phvikapen::core {

class StorageThread;

class AddTextCommand final : public ICommand {
public:
    AddTextCommand(Page* page, StorageThread* storage, PlacedText placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    PlacedText m_placed;
};

class RemoveTextCommand final : public ICommand {
public:
    RemoveTextCommand(Page* page, StorageThread* storage, const Uuid& textId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    Uuid m_textId;
    std::optional<PlacedText> m_removed;
};

// Everything about a box that can be changed while it stays where it is in the order of the page:
// what it says, where it sits, how wide it runs and the face it wears.
class ChangeTextCommand final : public ICommand {
public:
    ChangeTextCommand(Page* page, StorageThread* storage, TextBox box) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> write(TextBox box);

    Page* m_page;
    StorageThread* m_storage;
    TextBox m_after;
    std::optional<TextBox> m_before;
};

}
