#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/Picture.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>

namespace phvikapen::core {

class StorageThread;

class AddPictureCommand final : public ICommand {
public:
    AddPictureCommand(Page* page, StorageThread* storage, PlacedPicture placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    PlacedPicture m_placed;
};

class RemovePictureCommand final : public ICommand {
public:
    RemovePictureCommand(Page* page, StorageThread* storage, const Uuid& pictureId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    Uuid m_pictureId;
    std::optional<PlacedPicture> m_removed;
};

// Everything about a picture that can be changed while it keeps its place in the order of the
// page: where it stands, how large it is drawn and how far it has been turned.
class ChangePictureCommand final : public ICommand {
public:
    ChangePictureCommand(Page* page, StorageThread* storage, Picture picture) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> write(Picture picture);

    Page* m_page;
    StorageThread* m_storage;
    Picture m_after;
    std::optional<Picture> m_before;
};

}
