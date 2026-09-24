#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/TextBox.hpp"
#include "core/undo/UndoStack.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

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

class DuplicatePageCommand final : public ICommand {
public:
    DuplicatePageCommand(Outline* outline, StorageThread* storage, PagePlace place, PageInfo page,
                         std::vector<PlacedStroke> strokes, std::vector<PlacedText> texts) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    PagePlace m_place;
    PageInfo m_page;
    std::vector<PlacedStroke> m_strokes;
    std::vector<PlacedText> m_texts;
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

// The setup of a section: every page in it takes the same paper, and goes back together.
class SetSectionStyleCommand final : public ICommand {
public:
    SetSectionStyleCommand(Outline* outline, StorageThread* storage, std::vector<Uuid> pageIds,
                           const PageStyle& style);

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    std::vector<Uuid> m_pageIds;
    std::vector<PageStyle> m_wasStyle;
    PageStyle m_style;
};

class SetPageMediaCommand final : public ICommand {
public:
    SetPageMediaCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                        std::optional<PageMedia> media) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    Uuid m_pageId;
    std::optional<PageMedia> m_media;
};

class ImportPagesCommand final : public ICommand {
public:
    ImportPagesCommand(Outline* outline, StorageThread* storage, PagePlace place, Asset asset,
                       std::vector<PageInfo> pages) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Outline* m_outline;
    StorageThread* m_storage;
    PagePlace m_place;
    Asset m_asset;
    std::vector<PageInfo> m_pages;
    bool m_stored{false};
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
