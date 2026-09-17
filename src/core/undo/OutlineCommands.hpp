#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/undo/UndoStack.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace phvikapen::core {

class StorageThread;

class AddPageCommand final : public ICommand {
public:
    AddPageCommand(Outline* outline, StorageThread* storage, PagePlace place,
                   PageInfo page) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    PagePlace m_place;
    PageInfo m_page;
    bool m_stored{false};
};

class DeletePageCommand final : public ICommand {
public:
    DeletePageCommand(Outline* outline, StorageThread* storage, const Uuid& pageId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_pageId;
    std::optional<RemovedPage> m_removed;
};

class MovePageCommand final : public ICommand {
public:
    MovePageCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                    PagePlace place) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> moveTo(PagePlace place);

    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_pageId;
    PagePlace m_place;
    PagePlace m_previous;
};

class RenamePageCommand final : public ICommand {
public:
    RenamePageCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                      std::string title) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_pageId;
    std::string m_title;
};

class SetPageStyleCommand final : public ICommand {
public:
    SetPageStyleCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                        const PageStyle& style) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_pageId;
    PageStyle m_style;
};

class AddSectionCommand final : public ICommand {
public:
    AddSectionCommand(Outline* outline, StorageThread* storage, std::size_t index,
                      SectionInfo section) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    std::size_t m_index;
    SectionInfo m_section;
    bool m_stored{false};
};

class DeleteSectionCommand final : public ICommand {
public:
    DeleteSectionCommand(Outline* outline, StorageThread* storage, const Uuid& sectionId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_sectionId;
    std::optional<RemovedSection> m_removed;
};

class MoveSectionCommand final : public ICommand {
public:
    MoveSectionCommand(Outline* outline, StorageThread* storage, const Uuid& sectionId,
                       std::size_t index) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    [[nodiscard]] Result<void> moveTo(std::size_t index);

    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_sectionId;
    std::size_t m_index;
    std::size_t m_previous{};
};

class RenameSectionCommand final : public ICommand {
public:
    RenameSectionCommand(Outline* outline, StorageThread* storage, const Uuid& sectionId,
                         std::string title) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_sectionId;
    std::string m_title;
};

}
