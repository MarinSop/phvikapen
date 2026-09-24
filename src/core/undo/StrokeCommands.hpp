#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeTransform.hpp"
#include "core/model/Page.hpp"
#include "core/model/TextBox.hpp"
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
    std::vector<PlacedText> m_removedTexts;
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

// Moving, sizing and turning are one change: what the strokes were is kept, so that undoing puts
// them back exactly rather than working the sums backwards.
class TransformStrokesCommand final : public ICommand {
public:
    TransformStrokesCommand(Page* page, StorageThread* storage, std::vector<Uuid> strokeIds,
                            Transform transform) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<Uuid> m_strokeIds;
    Transform m_transform;
    std::vector<PlacedStroke> m_before;
};

class AddStrokesCommand final : public ICommand {
public:
    AddStrokesCommand(Page* page, StorageThread* storage,
                      std::vector<PlacedStroke> placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<PlacedStroke> m_placed;
};

class RestyleStrokesCommand final : public ICommand {
public:
    RestyleStrokesCommand(Page* page, StorageThread* storage, std::vector<Uuid> strokeIds,
                          StrokeStyle style) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<Uuid> m_strokeIds;
    StrokeStyle m_style;
    std::vector<PlacedStroke> m_before;
};

class SplitStrokesCommand final : public ICommand {
public:
    SplitStrokesCommand(Page* page, StorageThread* storage, std::vector<Uuid> strokeIds,
                        std::vector<PlacedStroke> pieces) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    std::vector<Uuid> m_strokeIds;
    std::vector<PlacedStroke> m_pieces;
    std::vector<PlacedStroke> m_erased;
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
