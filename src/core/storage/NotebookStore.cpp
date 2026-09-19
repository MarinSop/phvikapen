#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/Sqlite.hpp"
#include "core/storage/StrokeCodec.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

// Add new schema versions as new steps; never edit a released one.
constexpr std::string_view kSchemaVersion1 = R"sql(
    CREATE TABLE strokes (
        id        BLOB PRIMARY KEY NOT NULL,
        page_id   BLOB NOT NULL,
        ordinal   INTEGER NOT NULL,
        data      BLOB NOT NULL
    );
    CREATE INDEX strokes_by_page ON strokes (page_id, ordinal);
)sql";

constexpr std::string_view kSchemaVersion2 = R"sql(
    DROP INDEX strokes_by_page;
    CREATE UNIQUE INDEX strokes_by_page ON strokes (page_id, ordinal);
)sql";

constexpr std::string_view kSchemaVersion3 = R"sql(
    CREATE TABLE notebook (
        id    INTEGER PRIMARY KEY CHECK (id = 1),
        title TEXT NOT NULL
    );
    CREATE TABLE sections (
        id      BLOB PRIMARY KEY NOT NULL,
        ordinal INTEGER NOT NULL,
        title   TEXT NOT NULL,
        trashed INTEGER NOT NULL DEFAULT 0
    );
    CREATE TABLE pages (
        id          BLOB PRIMARY KEY NOT NULL,
        section_id  BLOB NOT NULL REFERENCES sections (id),
        ordinal     INTEGER NOT NULL,
        title       TEXT NOT NULL,
        paper       INTEGER NOT NULL,
        orientation INTEGER NOT NULL,
        background  INTEGER NOT NULL,
        spacing     REAL NOT NULL,
        trashed     INTEGER NOT NULL DEFAULT 0
    );
    CREATE INDEX pages_by_section ON pages (section_id, ordinal);
    INSERT INTO sections (id, ordinal, title)
        SELECT randomblob(16), 0, 'Section 1' WHERE EXISTS (SELECT 1 FROM strokes);
    INSERT INTO pages (id, section_id, ordinal, title, paper, orientation, background, spacing)
        SELECT page_id, (SELECT id FROM sections), ROW_NUMBER() OVER (ORDER BY MIN(rowid)) - 1, '',
               2, 0, 1, 26.456692913385826
        FROM strokes GROUP BY page_id;
)sql";

constexpr std::string_view kSchemaVersion4 = R"sql(
    CREATE TABLE assets (
        id   BLOB PRIMARY KEY NOT NULL,
        kind INTEGER NOT NULL,
        name TEXT NOT NULL,
        data BLOB NOT NULL
    );
    ALTER TABLE pages ADD COLUMN custom_width REAL NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN custom_height REAL NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN media_asset BLOB REFERENCES assets (id);
    ALTER TABLE pages ADD COLUMN media_index INTEGER NOT NULL DEFAULT 0;
)sql";

// The paper a page is written on: its colour, the colour and thickness of its ruling, and the
// line down the side. A colour of nothing means "whatever suits this ruling".
constexpr std::string_view kSchemaVersion5 = R"sql(
    ALTER TABLE pages ADD COLUMN paper_color INTEGER NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN line_color INTEGER NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN margin_color INTEGER NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN line_width REAL NOT NULL DEFAULT 1;
    ALTER TABLE pages ADD COLUMN margin_at REAL NOT NULL DEFAULT 94.4881889763779;
    ALTER TABLE pages ADD COLUMN margin INTEGER NOT NULL DEFAULT 1;
)sql";

constexpr std::array kMigrations{
    std::pair{1, kSchemaVersion1}, std::pair{2, kSchemaVersion2}, std::pair{3, kSchemaVersion3},
    std::pair{4, kSchemaVersion4}, std::pair{5, kSchemaVersion5},
};

constexpr std::string_view kDefaultSectionTitle = "Section 1";

[[nodiscard]] Paper toPaper(std::int64_t value) noexcept {
    return value >= 0 && value <= static_cast<std::int64_t>(Paper::Custom)
               ? static_cast<Paper>(value)
               : PageStyle{}.paper;
}

[[nodiscard]] Orientation toOrientation(std::int64_t value) noexcept {
    return value == static_cast<std::int64_t>(Orientation::Landscape) ? Orientation::Landscape
                                                                      : Orientation::Portrait;
}

[[nodiscard]] AssetKind toAssetKind(std::int64_t value) noexcept {
    return value == static_cast<std::int64_t>(AssetKind::Image) ? AssetKind::Image : AssetKind::Pdf;
}

[[nodiscard]] ContentId toContentId(std::span<const std::byte> stored) noexcept {
    ContentId::Bytes bytes{};
    if (stored.size() == bytes.size()) {
        std::ranges::transform(stored, bytes.begin(),
                               [](std::byte value) { return static_cast<std::uint8_t>(value); });
    }
    return ContentId{bytes};
}

[[nodiscard]] Background toBackground(std::int64_t value) noexcept {
    return value >= 0 && value <= static_cast<std::int64_t>(Background::Dotted)
               ? static_cast<Background>(value)
               : PageStyle{}.background;
}

[[nodiscard]] std::span<const std::byte> contentBytes(const ContentId& id) {
    return std::as_bytes(std::span{id.bytes()});
}

[[nodiscard]] Result<void> expectChange(sqlite3* database, std::string_view missing) {
    if (sqlite::changes(database) == 0) {
        return makeError(ErrorCode::NotFound, std::string{missing});
    }
    return {};
}

[[nodiscard]] Result<void> bindAll(std::initializer_list<Result<void>> bindings) {
    for (const Result<void>& bound : bindings) {
        if (!bound) {
            return bound;
        }
    }
    return {};
}

[[nodiscard]] Result<std::int64_t> queryInteger(sqlite3* database, std::string_view sql) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(database, sql);
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    const Result<bool> row = statement->step();
    if (!row) {
        return std::unexpected{row.error()};
    }
    return *row ? statement->integer(0) : 0;
}

[[nodiscard]] Result<void> migrate(sqlite3* database) {
    const Result<std::int64_t> version = queryInteger(database, "PRAGMA user_version;");
    if (!version) {
        return std::unexpected{version.error()};
    }
    if (*version > kNotebookSchemaVersion) {
        return makeError(ErrorCode::Unsupported,
                         "notebook was written by a newer version of the application");
    }
    if (*version == kNotebookSchemaVersion) {
        return {};
    }

    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    for (const auto& [stepVersion, sql] : kMigrations) {
        if (*version >= stepVersion) {
            continue;
        }
        if (const Result<void> applied = sqlite::execute(database, sql); !applied) {
            return applied;
        }
    }
    if (const Result<void> stamped = sqlite::execute(
            database, "PRAGMA user_version = " + std::to_string(kNotebookSchemaVersion) + ";");
        !stamped) {
        return stamped;
    }
    return transaction->commit();
}

}

NotebookStore::NotebookStore(sqlite3* database) noexcept : m_database{database} {}

NotebookStore::~NotebookStore() {
    close();
}

NotebookStore::NotebookStore(NotebookStore&& other) noexcept
    : m_database{std::exchange(other.m_database, nullptr)} {}

NotebookStore& NotebookStore::operator=(NotebookStore&& other) noexcept {
    if (this != &other) {
        close();
        m_database = std::exchange(other.m_database, nullptr);
    }
    return *this;
}

void NotebookStore::close() noexcept {
    if (m_database != nullptr) {
        sqlite3_close(m_database);
        m_database = nullptr;
    }
}

Result<NotebookStore> NotebookStore::open(const std::filesystem::path& path) {
    // SQLite expects UTF-8, which path::string() is not on Windows.
    const std::u8string utf8Path = path.u8string();
    const std::string fileName{utf8Path.begin(), utf8Path.end()};

    sqlite3* database = nullptr;
    const int status = sqlite3_open_v2(fileName.c_str(), &database,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (status != SQLITE_OK) {
        Error error = sqlite::lastError(database, "could not open the notebook");
        sqlite3_close(database);
        return std::unexpected{std::move(error)};
    }

    NotebookStore store{database};

    for (const std::string_view pragma : {
             "PRAGMA journal_mode = WAL;",
             "PRAGMA foreign_keys = ON;",
             "PRAGMA synchronous = NORMAL;",
         }) {
        if (const Result<void> applied = sqlite::execute(database, pragma); !applied) {
            return std::unexpected{applied.error()};
        }
    }

    if (const Result<void> migrated = migrate(database); !migrated) {
        return std::unexpected{migrated.error()};
    }
    const std::u8string stem = path.stem().u8string();
    if (const Result<void> ensured = store.ensureOutline(std::string{stem.begin(), stem.end()});
        !ensured) {
        return std::unexpected{ensured.error()};
    }
    return store;
}

Result<void> NotebookStore::insertStroke(const Uuid& pageId, const PlacedStroke& placed) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "INSERT INTO strokes (id, page_id, ordinal, data) VALUES (?, ?, ?, ?);");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    const std::vector<std::byte> data = encodeStroke(placed.stroke);
    for (const Result<void>& bound : {
             statement->bindId(1, placed.stroke.id()),
             statement->bindId(2, pageId),
             statement->bindInteger(3, placed.ordinal),
             statement->bindBlob(4, data),
         }) {
        if (!bound) {
            return bound;
        }
    }
    return statement->run();
}

Result<void> NotebookStore::removeStroke(const Uuid& pageId, const Uuid& strokeId) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "DELETE FROM strokes WHERE page_id = ? AND id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    for (const Result<void>& bound : {
             statement->bindId(1, pageId),
             statement->bindId(2, strokeId),
         }) {
        if (!bound) {
            return bound;
        }
    }
    if (const Result<void> removed = statement->run(); !removed) {
        return removed;
    }
    if (sqlite::changes(m_database) == 0) {
        return makeError(ErrorCode::NotFound, "the page does not hold that stroke");
    }
    return {};
}

Result<std::size_t> NotebookStore::removeStrokesOfPage(const Uuid& pageId) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "DELETE FROM strokes WHERE page_id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return std::unexpected{bound.error()};
    }
    if (const Result<void> removed = statement->run(); !removed) {
        return std::unexpected{removed.error()};
    }
    return static_cast<std::size_t>(sqlite::changes(m_database));
}

Result<std::vector<PlacedStroke>> NotebookStore::strokesOfPage(const Uuid& pageId) const {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "SELECT ordinal, data FROM strokes WHERE page_id = ? ORDER BY ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return std::unexpected{bound.error()};
    }

    std::vector<PlacedStroke> strokes;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            break;
        }
        Result<Stroke> stroke = decodeStroke(statement->blob(1));
        if (!stroke) {
            return std::unexpected{stroke.error()};
        }
        strokes.push_back(
            PlacedStroke{.ordinal = statement->integer(0), .stroke = std::move(*stroke)});
    }
    return strokes;
}

Result<void> NotebookStore::ensureOutline(std::string_view defaultTitle) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }

    Result<sqlite::Statement> title = sqlite::Statement::prepare(
        m_database, "INSERT OR IGNORE INTO notebook (id, title) VALUES (1, ?);");
    if (!title) {
        return std::unexpected{title.error()};
    }
    if (const Result<void> written =
            bindAll({title->bindText(1, defaultTitle)}).and_then([&] { return title->run(); });
        !written) {
        return written;
    }

    const Result<std::int64_t> sections =
        queryInteger(m_database, "SELECT COUNT(*) FROM sections WHERE trashed = 0;");
    if (!sections) {
        return std::unexpected{sections.error()};
    }
    if (*sections == 0) {
        Uuid7Generator ids;
        const Uuid sectionId = ids.next();
        const std::array sectionOrder{sectionId};
        if (const Result<void> inserted =
                insertSection(sectionId, kDefaultSectionTitle, sectionOrder);
            !inserted) {
            return inserted;
        }
    }

    Result<sqlite::Statement> empty = sqlite::Statement::prepare(
        m_database, "SELECT id FROM sections WHERE trashed = 0 AND NOT EXISTS "
                    "(SELECT 1 FROM pages WHERE pages.section_id = sections.id "
                    "AND pages.trashed = 0);");
    if (!empty) {
        return std::unexpected{empty.error()};
    }
    std::vector<Uuid> emptySections;
    while (true) {
        const Result<bool> row = empty->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            break;
        }
        emptySections.push_back(empty->id(0));
    }
    Uuid7Generator ids;
    for (const Uuid& sectionId : emptySections) {
        const PageStyle style;
        const PageInfo page{.id = ids.next(), .title = {}, .style = style, .media = std::nullopt};
        const std::array pageOrder{page.id};
        if (const Result<void> inserted = insertPage(sectionId, page, pageOrder); !inserted) {
            return inserted;
        }
    }
    return transaction->commit();
}

Result<NotebookOutline> NotebookStore::readOutline() const {
    NotebookOutline outline;

    Result<sqlite::Statement> title =
        sqlite::Statement::prepare(m_database, "SELECT title FROM notebook WHERE id = 1;");
    if (!title) {
        return std::unexpected{title.error()};
    }
    const Result<bool> titleRow = title->step();
    if (!titleRow) {
        return std::unexpected{titleRow.error()};
    }
    if (*titleRow) {
        outline.title = title->text(0);
    }

    Result<sqlite::Statement> sections = sqlite::Statement::prepare(
        m_database, "SELECT id, title FROM sections WHERE trashed = 0 ORDER BY ordinal, rowid;");
    if (!sections) {
        return std::unexpected{sections.error()};
    }
    while (true) {
        const Result<bool> row = sections->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            break;
        }
        outline.sections.push_back(
            SectionInfo{.id = sections->id(0), .title = sections->text(1), .pages = {}});
    }

    Result<sqlite::Statement> pages = sqlite::Statement::prepare(
        m_database,
        "SELECT id, title, paper, orientation, background, spacing, custom_width, custom_height, "
        "paper_color, line_color, margin_color, line_width, margin_at, margin, "
        "media_asset, media_index FROM pages WHERE section_id = ? AND trashed = 0 "
        "ORDER BY ordinal, rowid;");
    if (!pages) {
        return std::unexpected{pages.error()};
    }
    for (SectionInfo& section : outline.sections) {
        if (const Result<void> bound = bindAll({pages->reset(), pages->bindId(1, section.id)});
            !bound) {
            return std::unexpected{bound.error()};
        }
        while (true) {
            const Result<bool> row = pages->step();
            if (!row) {
                return std::unexpected{row.error()};
            }
            if (!*row) {
                break;
            }
            int column = 0;
            PageInfo page{
                .id = pages->id(column++),
                .title = pages->text(column++),
                .style = normalized(PageStyle{
                    .paper = toPaper(pages->integer(column++)),
                    .orientation = toOrientation(pages->integer(column++)),
                    .background = toBackground(pages->integer(column++)),
                    .spacing = static_cast<float>(pages->real(column++)),
                    .customWidth = static_cast<float>(pages->real(column++)),
                    .customHeight = static_cast<float>(pages->real(column++)),
                    .paperColor = unpacked(static_cast<std::uint32_t>(pages->integer(column++))),
                    .lineColor = unpacked(static_cast<std::uint32_t>(pages->integer(column++))),
                    .marginColor = unpacked(static_cast<std::uint32_t>(pages->integer(column++))),
                    .lineWidth = static_cast<float>(pages->real(column++)),
                    .marginAt = static_cast<float>(pages->real(column++)),
                    .margin = pages->integer(column++) != 0,
                }),
                .media = std::nullopt,
            };
            const ContentId asset = toContentId(pages->blob(column++));
            const auto mediaIndex = static_cast<int>(pages->integer(column++));
            if (!asset.isEmpty()) {
                page.media = PageMedia{.asset = asset, .index = mediaIndex};
            }
            section.pages.push_back(std::move(page));
        }
    }
    return outline;
}

Result<void> NotebookStore::setTitle(std::string_view title) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE notebook SET title = ? WHERE id = 1;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindText(1, title)}).and_then([&] { return statement->run(); });
}

Result<void> NotebookStore::writeSectionOrder(std::span<const Uuid> sectionOrder) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE sections SET ordinal = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    std::int64_t ordinal = 0;
    for (const Uuid& sectionId : sectionOrder) {
        if (const Result<void> written =
                bindAll({
                            statement->reset(),
                            statement->bindInteger(1, ordinal++),
                            statement->bindId(2, sectionId),
                        })
                    .and_then([&] { return statement->run(); })
                    .and_then([&] { return expectChange(m_database, "no such section"); });
            !written) {
            return written;
        }
    }
    return {};
}

Result<void> NotebookStore::writePageOrder(const Uuid& sectionId, std::span<const Uuid> pageOrder) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "UPDATE pages SET section_id = ?, ordinal = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    std::int64_t ordinal = 0;
    for (const Uuid& pageId : pageOrder) {
        if (const Result<void> written =
                bindAll({
                            statement->reset(),
                            statement->bindId(1, sectionId),
                            statement->bindInteger(2, ordinal++),
                            statement->bindId(3, pageId),
                        })
                    .and_then([&] { return statement->run(); })
                    .and_then([&] { return expectChange(m_database, "no such page"); });
            !written) {
            return written;
        }
    }
    return {};
}

Result<void> NotebookStore::insertSection(const Uuid& sectionId, std::string_view title,
                                          std::span<const Uuid> sectionOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "INSERT INTO sections (id, ordinal, title) VALUES (?, 0, ?);");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindId(1, sectionId), statement->bindText(2, title)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return writeSectionOrder(sectionOrder); })
        .and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::renameSection(const Uuid& sectionId, std::string_view title) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE sections SET title = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindText(1, title), statement->bindId(2, sectionId)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such section"); });
}

Result<void> NotebookStore::trashSection(const Uuid& sectionId) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE sections SET trashed = 1 WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindId(1, sectionId)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such section"); });
}

Result<void> NotebookStore::restoreSection(const Uuid& sectionId,
                                           std::span<const Uuid> sectionOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE sections SET trashed = 0 WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindId(1, sectionId)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such section"); })
        .and_then([&] { return writeSectionOrder(sectionOrder); })
        .and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::orderSections(std::span<const Uuid> sectionOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    return writeSectionOrder(sectionOrder).and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::writePage(const Uuid& sectionId, const PageInfo& page) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database,
        "INSERT INTO pages (id, section_id, ordinal, title, paper, orientation, background, "
        "spacing, custom_width, custom_height, paper_color, line_color, margin_color, "
        "line_width, margin_at, margin, media_asset, media_index) "
        "VALUES (?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    int index = 1;
    return bindAll({
                       statement->bindId(index++, page.id),
                       statement->bindId(index++, sectionId),
                       statement->bindText(index++, page.title),
                       statement->bindInteger(index++, static_cast<std::int64_t>(page.style.paper)),
                       statement->bindInteger(index++,
                                              static_cast<std::int64_t>(page.style.orientation)),
                       statement->bindInteger(index++,
                                              static_cast<std::int64_t>(page.style.background)),
                       statement->bindReal(index++, page.style.spacing),
                       statement->bindReal(index++, page.style.customWidth),
                       statement->bindReal(index++, page.style.customHeight),
                       statement->bindInteger(index++, packed(page.style.paperColor)),
                       statement->bindInteger(index++, packed(page.style.lineColor)),
                       statement->bindInteger(index++, packed(page.style.marginColor)),
                       statement->bindReal(index++, page.style.lineWidth),
                       statement->bindReal(index++, page.style.marginAt),
                       statement->bindInteger(index++, page.style.margin ? 1 : 0),
                       page.media ? statement->bindBlob(index++, contentBytes(page.media->asset))
                                  : statement->bindNull(index++),
                       statement->bindInteger(index++, page.media ? page.media->index : 0),
                   })
        .and_then([&] { return statement->run(); });
}

Result<void> NotebookStore::insertPage(const Uuid& sectionId, const PageInfo& page,
                                       std::span<const Uuid> pageOrder) {
    const std::array pages{page};
    return insertPages(sectionId, pages, pageOrder);
}

Result<void> NotebookStore::insertPages(const Uuid& sectionId, std::span<const PageInfo> pages,
                                        std::span<const Uuid> pageOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    for (const PageInfo& page : pages) {
        if (const Result<void> written = writePage(sectionId, page); !written) {
            return written;
        }
    }
    return writePageOrder(sectionId, pageOrder).and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::renamePage(const Uuid& pageId, std::string_view title) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE pages SET title = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindText(1, title), statement->bindId(2, pageId)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such page"); });
}

Result<void> NotebookStore::setPageStyle(const Uuid& pageId, const PageStyle& style) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "UPDATE pages SET paper = ?, orientation = ?, background = ?, spacing = ?, "
                    "custom_width = ?, custom_height = ?, paper_color = ?, line_color = ?, "
                    "margin_color = ?, line_width = ?, margin_at = ?, margin = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    int index = 1;
    return bindAll(
               {
                   statement->bindInteger(index++, static_cast<std::int64_t>(style.paper)),
                   statement->bindInteger(index++, static_cast<std::int64_t>(style.orientation)),
                   statement->bindInteger(index++, static_cast<std::int64_t>(style.background)),
                   statement->bindReal(index++, style.spacing),
                   statement->bindReal(index++, style.customWidth),
                   statement->bindReal(index++, style.customHeight),
                   statement->bindInteger(index++, packed(style.paperColor)),
                   statement->bindInteger(index++, packed(style.lineColor)),
                   statement->bindInteger(index++, packed(style.marginColor)),
                   statement->bindReal(index++, style.lineWidth),
                   statement->bindReal(index++, style.marginAt),
                   statement->bindInteger(index++, style.margin ? 1 : 0),
                   statement->bindId(index++, pageId),
               })
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such page"); });
}

Result<void> NotebookStore::setPageMedia(const Uuid& pageId,
                                         const std::optional<PageMedia>& media) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "UPDATE pages SET media_asset = ?, media_index = ? WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    int index = 1;
    return bindAll({
                       media ? statement->bindBlob(index++, contentBytes(media->asset))
                             : statement->bindNull(index++),
                       statement->bindInteger(index++, media ? media->index : 0),
                       statement->bindId(index++, pageId),
                   })
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such page"); });
}

Result<std::vector<TrashedItem>> NotebookStore::trashedItems() const {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database,
        "SELECT id, id, title, 1, 0 FROM sections WHERE trashed = 1 "
        "UNION ALL "
        "SELECT pages.id, pages.section_id, pages.title, 0, sections.trashed FROM pages "
        "JOIN sections ON sections.id = pages.section_id WHERE pages.trashed = 1 "
        "ORDER BY 4 DESC, 3;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }

    std::vector<TrashedItem> items;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            break;
        }
        items.push_back(TrashedItem{
            .id = statement->id(0),
            .sectionId = statement->id(1),
            .title = statement->text(2),
            .wholeSection = statement->integer(3) != 0,
            .sectionTrashed = statement->integer(4) != 0,
        });
    }
    return items;
}

Result<void> NotebookStore::emptyTrash() {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }

    constexpr std::string_view kGone =
        "DELETE FROM strokes WHERE page_id IN (SELECT id FROM pages WHERE trashed = 1 "
        "  OR section_id IN (SELECT id FROM sections WHERE trashed = 1));"
        "DELETE FROM pages WHERE trashed = 1 "
        "  OR section_id IN (SELECT id FROM sections WHERE trashed = 1);"
        "DELETE FROM sections WHERE trashed = 1;"
        "DELETE FROM assets WHERE id NOT IN "
        "  (SELECT media_asset FROM pages WHERE media_asset IS NOT NULL);";
    if (const Result<void> removed = sqlite::execute(m_database, kGone); !removed) {
        return removed;
    }
    if (const Result<void> committed = transaction->commit(); !committed) {
        return committed;
    }
    return sqlite::execute(m_database, "VACUUM;");
}

Result<void> NotebookStore::insertAsset(const Asset& asset) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "INSERT OR IGNORE INTO assets (id, kind, name, data) VALUES (?, ?, ?, ?);");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    int index = 1;
    return bindAll({
                       statement->bindBlob(index++, contentBytes(asset.id)),
                       statement->bindInteger(index++, static_cast<std::int64_t>(asset.kind)),
                       statement->bindText(index++, asset.name),
                       statement->bindBlob(index++, asset.data),
                   })
        .and_then([&] { return statement->run(); });
}

Result<Asset> NotebookStore::asset(const ContentId& assetId) const {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "SELECT kind, name, data FROM assets WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindBlob(1, contentBytes(assetId)); !bound) {
        return std::unexpected{bound.error()};
    }
    const Result<bool> row = statement->step();
    if (!row) {
        return std::unexpected{row.error()};
    }
    if (!*row) {
        return makeError(ErrorCode::NotFound, "the notebook has no such file");
    }
    int column = 0;
    const AssetKind kind = toAssetKind(statement->integer(column++));
    std::string name = statement->text(column++);
    const std::span<const std::byte> data = statement->blob(column++);
    return Asset{
        .id = assetId,
        .kind = kind,
        .name = std::move(name),
        .data = std::vector<std::byte>{data.begin(), data.end()},
    };
}

Result<void> NotebookStore::trashPage(const Uuid& pageId) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE pages SET trashed = 1 WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    return bindAll({statement->bindId(1, pageId)})
        .and_then([&] { return statement->run(); })
        .and_then([&] { return expectChange(m_database, "no such page"); });
}

Result<void> NotebookStore::restorePage(const Uuid& sectionId, const Uuid& pageId,
                                        std::span<const Uuid> pageOrder) {
    const std::array pages{pageId};
    return restorePages(sectionId, pages, pageOrder);
}

Result<void> NotebookStore::restorePages(const Uuid& sectionId, std::span<const Uuid> pageIds,
                                         std::span<const Uuid> pageOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "UPDATE pages SET trashed = 0 WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    for (const Uuid& pageId : pageIds) {
        if (const Result<void> restored =
                bindAll({statement->reset(), statement->bindId(1, pageId)})
                    .and_then([&] { return statement->run(); })
                    .and_then([&] { return expectChange(m_database, "no such page"); });
            !restored) {
            return restored;
        }
    }
    return writePageOrder(sectionId, pageOrder).and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::orderPages(const Uuid& sectionId, std::span<const Uuid> pageOrder) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }
    return writePageOrder(sectionId, pageOrder).and_then([&] { return transaction->commit(); });
}

Result<void> NotebookStore::checkpoint() {
    return sqlite::execute(m_database, "PRAGMA wal_checkpoint(TRUNCATE);");
}

Result<int> NotebookStore::schemaVersion() const {
    const Result<std::int64_t> version = queryInteger(m_database, "PRAGMA user_version;");
    if (!version) {
        return std::unexpected{version.error()};
    }
    return static_cast<int>(*version);
}

}
