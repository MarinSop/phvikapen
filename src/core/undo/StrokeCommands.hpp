#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>
#include <vector>

namespace phvikapen::core {

class StorageThread;

class AddStrokeCommand final : public ICommand {
public:
    AddStrokeCommand(Page* page, StorageThread* storage, PlacedStroke placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

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
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<PlacedStroke> m_removed;
};

class MoveStrokesCommand final : public ICommand {
public:
    MoveStrokesCommand(Page* page, StorageThread* storage, std::vector<Uuid> strokeIds, float dx,
                       float dy) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> shift(float dx, float dy);

    Page* m_page;
    StorageThread* m_storage;
    std::vector<Uuid> m_strokeIds;
    float m_dx;
    float m_dy;
};

class EraseStrokesCommand final : public ICommand {
public:
    EraseStrokesCommand(Page* page, StorageThread* storage, std::vector<Uuid> strokeIds) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<Uuid> m_strokeIds;
    std::vector<PlacedStroke> m_erased;
};

}
