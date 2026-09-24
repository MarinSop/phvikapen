#include "core/undo/PictureCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/Picture.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <optional>

namespace phvikapen::core {

AddPictureCommand::AddPictureCommand(Page* page, StorageThread* storage,
                                     PlacedPicture placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{placed} {}

Result<void> AddPictureCommand::apply() {
    if (const Result<void> inserted = m_page->insertPicture(m_placed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = m_placed](NotebookStore& store) {
        return store.insertPicture(pageId, placed);
    });
    return {};
}

Result<void> AddPictureCommand::revert() {
    if (const Result<PlacedPicture> removed = m_page->removePicture(m_placed.picture.id);
        !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit([pageId = m_page->id(), pictureId = m_placed.picture.id](
                          NotebookStore& store) { return store.removePicture(pageId, pictureId); });
    return {};
}

std::optional<Uuid> AddPictureCommand::pageToShow() const {
    return m_page->id();
}

RemovePictureCommand::RemovePictureCommand(Page* page, StorageThread* storage,
                                           const Uuid& pictureId) noexcept
    : m_page{page}, m_storage{storage}, m_pictureId{pictureId} {}

Result<void> RemovePictureCommand::apply() {
    Result<PlacedPicture> removed = m_page->removePicture(m_pictureId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = *removed;
    m_storage->submit([pageId = m_page->id(), pictureId = m_pictureId](NotebookStore& store) {
        return store.removePicture(pageId, pictureId);
    });
    return {};
}

Result<void> RemovePictureCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::InvalidArgument, "there is no picture to put back");
    }
    if (const Result<void> inserted = m_page->insertPicture(*m_removed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = *m_removed](NotebookStore& store) {
        return store.insertPicture(pageId, placed);
    });
    return {};
}

std::optional<Uuid> RemovePictureCommand::pageToShow() const {
    return m_page->id();
}

ChangePictureCommand::ChangePictureCommand(Page* page, StorageThread* storage,
                                           Picture picture) noexcept
    : m_page{page}, m_storage{storage}, m_after{picture} {}

Result<void> ChangePictureCommand::write(Picture picture) {
    if (const Result<void> written = m_page->replacePicture(picture); !written) {
        return written;
    }
    m_storage->submit([pageId = m_page->id(), picture](NotebookStore& store) {
        return store.updatePicture(pageId, picture);
    });
    return {};
}

Result<void> ChangePictureCommand::apply() {
    if (!m_before) {
        const Picture* const kept = m_page->pictureAt(m_after.id);
        if (kept == nullptr) {
            return makeError(ErrorCode::NotFound, "the page does not hold that picture");
        }
        m_before = *kept;
    }
    return write(m_after);
}

Result<void> ChangePictureCommand::revert() {
    if (!m_before) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing to go back to");
    }
    return write(*m_before);
}

std::optional<Uuid> ChangePictureCommand::pageToShow() const {
    return m_page->id();
}

}
