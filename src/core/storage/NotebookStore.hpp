#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Picture.hpp"
#include "core/model/Recording.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"
#include "core/text/InkWord.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace phvikapen::core {

inline constexpr int kNotebookSchemaVersion = 14;

struct TrashedItem {
    Uuid id;
    Uuid sectionId;
    std::string title;
    bool wholeSection{};
    bool sectionTrashed{};

    friend bool operator==(const TrashedItem&, const TrashedItem&) = default;
};

class NotebookStore {
public:
    [[nodiscard]] static Result<NotebookStore> open(const std::filesystem::path& path);

    ~NotebookStore();

    NotebookStore(NotebookStore&& other) noexcept;
    NotebookStore& operator=(NotebookStore&& other) noexcept;

    NotebookStore(const NotebookStore&) = delete;
    NotebookStore& operator=(const NotebookStore&) = delete;

    [[nodiscard]] Result<void> insertStroke(const Uuid& pageId, const PlacedStroke& placed);

    [[nodiscard]] Result<void> removeStroke(const Uuid& pageId, const Uuid& strokeId);

    [[nodiscard]] Result<std::size_t> removeStrokesOfPage(const Uuid& pageId);

    [[nodiscard]] Result<std::vector<PlacedStroke>> strokesOfPage(const Uuid& pageId) const;

    [[nodiscard]] Result<std::int64_t> inkRevisionOfPage(const Uuid& pageId) const;
    [[nodiscard]] Result<std::vector<Uuid>> pagesWaitingToBeRead() const;
    [[nodiscard]] Result<void> setWordsOfPage(const Uuid& pageId, std::int64_t inkRevision,
                                              std::span<const InkWord> words);
    [[nodiscard]] Result<std::vector<InkWord>> wordsOfPage(const Uuid& pageId) const;
    [[nodiscard]] Result<std::vector<FoundWord>> findWords(std::string_view text) const;

    [[nodiscard]] Result<void> insertText(const Uuid& pageId, const PlacedText& placed);
    [[nodiscard]] Result<void> updateText(const Uuid& pageId, const TextBox& box);
    [[nodiscard]] Result<void> removeText(const Uuid& pageId, const Uuid& textId);
    [[nodiscard]] Result<std::size_t> removeTextsOfPage(const Uuid& pageId);
    [[nodiscard]] Result<std::vector<PlacedText>> textsOfPage(const Uuid& pageId) const;

    [[nodiscard]] Result<void> insertPicture(const Uuid& pageId, const PlacedPicture& placed);
    [[nodiscard]] Result<void> updatePicture(const Uuid& pageId, const Picture& picture);
    [[nodiscard]] Result<void> removePicture(const Uuid& pageId, const Uuid& pictureId);
    [[nodiscard]] Result<std::size_t> removePicturesOfPage(const Uuid& pageId);
    [[nodiscard]] Result<std::vector<PlacedPicture>> picturesOfPage(const Uuid& pageId) const;

    [[nodiscard]] Result<void> insertTable(const Uuid& pageId, const PlacedTable& placed);
    [[nodiscard]] Result<void> updateTable(const Uuid& pageId, const Table& table);
    [[nodiscard]] Result<void> removeTable(const Uuid& pageId, const Uuid& tableId);
    [[nodiscard]] Result<std::size_t> removeTablesOfPage(const Uuid& pageId);
    [[nodiscard]] Result<std::vector<PlacedTable>> tablesOfPage(const Uuid& pageId) const;

    [[nodiscard]] Result<void> insertRecording(const Uuid& pageId, const Recording& recording);
    [[nodiscard]] Result<void> updateRecording(const Recording& recording);
    [[nodiscard]] Result<void> removeRecording(const Uuid& recordingId);
    [[nodiscard]] Result<std::vector<Recording>> recordingsOfPage(const Uuid& pageId) const;

    // What was said in a recording, in place of whatever was written down before.
    [[nodiscard]] Result<void> writeSayings(const Uuid& recordingId,
                                            std::span<const Saying> sayings);

    [[nodiscard]] Result<void> markThing(const Uuid& pageId, const Mark& mark);
    [[nodiscard]] Result<void> unmarkThing(const Uuid& thingId);
    [[nodiscard]] Result<std::vector<Mark>> marksOfPage(const Uuid& pageId) const;

    // The layers of a page, bottom first. A page written down before there were layers has none,
    // and everything on it stands on the one it is given when it is read.
    [[nodiscard]] Result<std::vector<Layer>> layersOfPage(const Uuid& pageId) const;

    // The layers of a page as they now stand, in place of whatever was written down before.
    [[nodiscard]] Result<void> writeLayers(const Uuid& pageId, std::span<const Layer> layers);

    // Which layer one thing standing on a page belongs to.
    [[nodiscard]] Result<void> moveToLayer(const Uuid& pageId, const Uuid& thingId,
                                           const Uuid& layerId);

    [[nodiscard]] Result<NotebookOutline> readOutline() const;

    [[nodiscard]] Result<void> setTitle(std::string_view title);

    [[nodiscard]] Result<void> insertSection(const Uuid& sectionId, std::string_view title,
                                             std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> renameSection(const Uuid& sectionId, std::string_view title);
    [[nodiscard]] Result<void> trashSection(const Uuid& sectionId);
    [[nodiscard]] Result<void> restoreSection(const Uuid& sectionId,
                                              std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> orderSections(std::span<const Uuid> sectionOrder);

    [[nodiscard]] Result<void> insertPage(const Uuid& sectionId, const PageInfo& page,
                                          std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> insertPages(const Uuid& sectionId, std::span<const PageInfo> pages,
                                           std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> renamePage(const Uuid& pageId, std::string_view title);
    [[nodiscard]] Result<void> setPageStyle(const Uuid& pageId, const PageStyle& style);
    [[nodiscard]] Result<void> setPageMedia(const Uuid& pageId,
                                            const std::optional<PageMedia>& media);
    [[nodiscard]] Result<void> trashPage(const Uuid& pageId);
    [[nodiscard]] Result<void> restorePage(const Uuid& sectionId, const Uuid& pageId,
                                           std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> restorePages(const Uuid& sectionId, std::span<const Uuid> pageIds,
                                            std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> orderPages(const Uuid& sectionId, std::span<const Uuid> pageOrder);

    [[nodiscard]] Result<std::vector<TrashedItem>> trashedItems() const;
    [[nodiscard]] Result<void> emptyTrash();

    [[nodiscard]] Result<void> insertAsset(const Asset& asset);
    [[nodiscard]] Result<Asset> asset(const ContentId& assetId) const;

    [[nodiscard]] Result<void> checkpoint();

    [[nodiscard]] Result<int> schemaVersion() const;

private:
    explicit NotebookStore(sqlite3* database) noexcept;

    void close() noexcept;

    [[nodiscard]] Result<void> ensureOutline(std::string_view defaultTitle);
    [[nodiscard]] Result<void> touchInk(const Uuid& pageId);
    [[nodiscard]] Result<void> writeSectionOrder(std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> writePageOrder(const Uuid& sectionId,
                                              std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> writePage(const Uuid& sectionId, const PageInfo& page);

    sqlite3* m_database{nullptr};
};

}
