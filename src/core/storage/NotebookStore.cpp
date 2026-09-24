#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/Sqlite.hpp"
#include "core/storage/StrokeCodec.hpp"
#include "core/text/Folding.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
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

// The words a page was read into, and how far the reading got: a page whose ink has moved on from
// what was read waits to be read again.
constexpr std::string_view kSchemaVersion6 = R"sql(
    CREATE TABLE page_words (
        page_id      BLOB NOT NULL REFERENCES pages (id),
        ordinal      INTEGER NOT NULL,
        text         TEXT NOT NULL,
        folded       TEXT NOT NULL,
        left_edge    REAL NOT NULL,
        top_edge     REAL NOT NULL,
        right_edge   REAL NOT NULL,
        bottom_edge  REAL NOT NULL,
        strokes      BLOB NOT NULL,
        PRIMARY KEY (page_id, ordinal)
    );
    CREATE INDEX page_words_by_word ON page_words (folded);
    ALTER TABLE pages ADD COLUMN ink_revision INTEGER NOT NULL DEFAULT 0;
    ALTER TABLE pages ADD COLUMN read_revision INTEGER NOT NULL DEFAULT 0;
    UPDATE pages SET read_revision = -1 WHERE EXISTS
        (SELECT 1 FROM strokes WHERE strokes.page_id = pages.id);
)sql";

// Text that was typed on a page rather than written: where it sits, how wide it may run, and the
// face it wears.
constexpr std::string_view kSchemaVersion7 = R"sql(
    CREATE TABLE page_texts (
        id          BLOB PRIMARY KEY NOT NULL,
        page_id     BLOB NOT NULL REFERENCES pages (id),
        ordinal     INTEGER NOT NULL,
        left_edge   REAL NOT NULL,
        top_edge    REAL NOT NULL,
        width       REAL NOT NULL,
        height      REAL NOT NULL,
        text        TEXT NOT NULL,
        folded      TEXT NOT NULL,
        font        TEXT NOT NULL,
        size        REAL NOT NULL,
        color       INTEGER NOT NULL,
        align       INTEGER NOT NULL,
        line_height REAL NOT NULL,
        marks       INTEGER NOT NULL
    );
    CREATE UNIQUE INDEX page_texts_by_page ON page_texts (page_id, ordinal);
    CREATE INDEX page_texts_by_word ON page_texts (folded);
)sql";

constexpr std::array kMigrations{
    std::pair{1, kSchemaVersion1}, std::pair{2, kSchemaVersion2}, std::pair{3, kSchemaVersion3},
    std::pair{4, kSchemaVersion4}, std::pair{6, kSchemaVersion6}, std::pair{7, kSchemaVersion7},
};

// The paper a page is written on: its colour, the colour and thickness of its ruling, and the line
// down the side. These went out while the version number stood still, so a notebook stamped with
// the version before them may carry them or not, and both are put right by adding what is missing.
constexpr std::array<std::pair<std::string_view, std::string_view>, 6> kPaperColumns{
    std::pair<std::string_view, std::string_view>{"paper_color", "INTEGER NOT NULL DEFAULT 0"},
    std::pair<std::string_view, std::string_view>{"line_color", "INTEGER NOT NULL DEFAULT 0"},
    std::pair<std::string_view, std::string_view>{"margin_color", "INTEGER NOT NULL DEFAULT 0"},
    std::pair<std::string_view, std::string_view>{"line_width", "REAL NOT NULL DEFAULT 1"},
    std::pair<std::string_view, std::string_view>{"margin_at",
                                                  "REAL NOT NULL DEFAULT 94.4881889763779"},
    std::pair<std::string_view, std::string_view>{"margin", "INTEGER NOT NULL DEFAULT 1"},
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

[[nodiscard]] std::vector<std::string> foldedWords(std::string_view text) {
    std::vector<std::string> words;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t space = text.find(' ', at);
        const std::string word = folded(text.substr(at, space - at));
        if (!word.empty()) {
            words.push_back(word);
        }
        if (space == std::string_view::npos) {
            break;
        }
        at = space + 1;
    }
    return words;
}

[[nodiscard]] bool runFollows(const std::vector<InkWord>& words, std::size_t from,
                              const std::vector<std::string>& wanted) {
    if (from + wanted.size() > words.size()) {
        return false;
    }
    for (std::size_t step = 1; step < wanted.size(); ++step) {
        if (!folded(words[from + step].text).contains(wanted[step])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string escapedForLike(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char letter : text) {
        if (letter == '%' || letter == '_' || letter == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(letter);
    }
    return escaped;
}

[[nodiscard]] InkWord wordFrom(const sqlite::Statement& statement, int first) {
    InkWord word{
        .text = statement.text(first),
        .box =
            Rect{
                .left = static_cast<float>(statement.real(first + 1)),
                .top = static_cast<float>(statement.real(first + 2)),
                .right = static_cast<float>(statement.real(first + 3)),
                .bottom = static_cast<float>(statement.real(first + 4)),
            },
        .strokes = {},
    };
    const std::span<const std::byte> stored = statement.blob(first + 5);
    for (std::size_t at = 0; at + Uuid::kByteCount <= stored.size(); at += Uuid::kByteCount) {
        Uuid::Bytes bytes{};
        std::ranges::transform(stored.subspan(at, Uuid::kByteCount), bytes.begin(),
                               [](std::byte value) { return static_cast<std::uint8_t>(value); });
        word.strokes.emplace_back(bytes);
    }
    return word;
}

constexpr std::uint64_t kBoldMark = 1U;
constexpr std::uint64_t kItalicMark = 2U;
constexpr std::uint64_t kUnderlineMark = 4U;
constexpr std::uint64_t kStruckMark = 8U;

// Where each part of a typed text sits in the row it is read from.
enum class TextColumn : std::uint8_t {
    Id,
    Ordinal,
    Left,
    Top,
    Width,
    Height,
    Text,
    Font,
    Size,
    Color,
    Align,
    LineHeight,
    Marks,
};

[[nodiscard]] constexpr int column(TextColumn which) noexcept {
    return static_cast<int>(which);
}

[[nodiscard]] std::int64_t marksOf(const TextStyle& style) noexcept {
    const std::uint64_t marks = (style.bold ? kBoldMark : 0U) | (style.italic ? kItalicMark : 0U)
                                | (style.underline ? kUnderlineMark : 0U)
                                | (style.struckOut ? kStruckMark : 0U);
    return static_cast<std::int64_t>(marks);
}

[[nodiscard]] TextAlign toAlign(std::int64_t value) noexcept {
    return value >= 0 && value <= static_cast<std::int64_t>(TextAlign::Justify)
               ? static_cast<TextAlign>(value)
               : TextAlign::Left;
}

// What a search runs against: the words of the text, plain and apart, so a phrase can be looked
// for the same way as one written by hand.
[[nodiscard]] std::string foldedRun(std::string_view text) {
    std::string run;
    for (const std::string& word : foldedWords(text)) {
        if (!run.empty()) {
            run.push_back(' ');
        }
        run.append(word);
    }
    return run;
}

[[nodiscard]] PlacedText textFrom(const sqlite::Statement& statement) {
    const auto marks = static_cast<std::uint64_t>(statement.integer(column(TextColumn::Marks)));
    return PlacedText{
        .ordinal = statement.integer(column(TextColumn::Ordinal)),
        .box =
            TextBox{
                .id = statement.id(column(TextColumn::Id)),
                .at =
                    Point{
                        .x = static_cast<float>(statement.real(column(TextColumn::Left))),
                        .y = static_cast<float>(statement.real(column(TextColumn::Top))),
                    },
                .width = static_cast<float>(statement.real(column(TextColumn::Width))),
                .height = static_cast<float>(statement.real(column(TextColumn::Height))),
                .text = statement.text(column(TextColumn::Text)),
                .style =
                    TextStyle{
                        .font = statement.text(column(TextColumn::Font)),
                        .size = static_cast<float>(statement.real(column(TextColumn::Size))),
                        .color = unpacked(static_cast<std::uint32_t>(
                            statement.integer(column(TextColumn::Color)))),
                        .align = toAlign(statement.integer(column(TextColumn::Align))),
                        .lineHeight =
                            static_cast<float>(statement.real(column(TextColumn::LineHeight))),
                        .bold = (marks & kBoldMark) != 0U,
                        .italic = (marks & kItalicMark) != 0U,
                        .underline = (marks & kUnderlineMark) != 0U,
                        .struckOut = (marks & kStruckMark) != 0U,
                    },
            },
    };
}

// Where the place of a hit sits in the rows a search reads.
enum class WrittenColumn : std::uint8_t {
    PageId,
    Ordinal,
    Text,
    Left,
    Top,
    Right,
    Bottom,
    Strokes,
    Section,
    Page,
};

enum class TypedColumn : std::uint8_t {
    PageId,
    Text,
    Left,
    Top,
    Width,
    Height,
    Section,
    Page,
};

[[nodiscard]] constexpr int column(WrittenColumn which) noexcept {
    return static_cast<int>(which);
}

[[nodiscard]] constexpr int column(TypedColumn which) noexcept {
    return static_cast<int>(which);
}

// A hit, with the place in the notebook it came from, so that what was written and what was typed
// can be listed in the order a reader turns the pages.
struct Hit {
    std::int64_t section{};
    std::int64_t page{};
    FoundWord found;
};

[[nodiscard]] Result<std::vector<Hit>> writtenHits(sqlite3* database, const NotebookStore& store,
                                                   const std::vector<std::string>& wanted) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        database,
        "SELECT page_words.page_id, page_words.ordinal, page_words.text, page_words.left_edge, "
        "       page_words.top_edge, page_words.right_edge, page_words.bottom_edge, "
        "       page_words.strokes, sections.ordinal, pages.ordinal "
        "FROM page_words "
        "JOIN pages ON pages.id = page_words.page_id "
        "JOIN sections ON sections.id = pages.section_id "
        "WHERE pages.trashed = 0 AND sections.trashed = 0 "
        "  AND page_words.folded LIKE ? ESCAPE '\\' "
        "ORDER BY sections.ordinal, pages.ordinal, page_words.ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound =
            statement->bindText(1, "%" + escapedForLike(wanted.front()) + "%");
        !bound) {
        return std::unexpected{bound.error()};
    }

    std::vector<Hit> hits;
    std::map<Uuid, std::vector<InkWord>> read;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return hits;
        }
        const Uuid pageId = statement->id(column(WrittenColumn::PageId));
        const auto ordinal =
            static_cast<std::size_t>(statement->integer(column(WrittenColumn::Ordinal)));
        InkWord word = wordFrom(*statement, column(WrittenColumn::Text));
        if (wanted.size() > 1) {
            if (!read.contains(pageId)) {
                Result<std::vector<InkWord>> words = store.wordsOfPage(pageId);
                if (!words) {
                    return std::unexpected{words.error()};
                }
                read.emplace(pageId, std::move(*words));
            }
            // The words after the first have to follow it, so that a search of several words finds
            // them written one after another.
            if (!runFollows(read.at(pageId), ordinal, wanted)) {
                continue;
            }
        }
        hits.push_back(Hit{
            .section = statement->integer(column(WrittenColumn::Section)),
            .page = statement->integer(column(WrittenColumn::Page)),
            .found = FoundWord{.pageId = pageId, .word = std::move(word)},
        });
    }
}

[[nodiscard]] Result<std::vector<Hit>> typedHits(sqlite3* database,
                                                 const std::vector<std::string>& wanted) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        database, "SELECT page_texts.page_id, page_texts.text, page_texts.left_edge, "
                  "       page_texts.top_edge, page_texts.width, page_texts.height, "
                  "       sections.ordinal, pages.ordinal "
                  "FROM page_texts "
                  "JOIN pages ON pages.id = page_texts.page_id "
                  "JOIN sections ON sections.id = pages.section_id "
                  "WHERE pages.trashed = 0 AND sections.trashed = 0 "
                  "  AND page_texts.folded LIKE ? ESCAPE '\\' "
                  "ORDER BY sections.ordinal, pages.ordinal, page_texts.ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    std::string phrase;
    for (const std::string& word : wanted) {
        if (!phrase.empty()) {
            phrase.push_back(' ');
        }
        phrase.append(escapedForLike(word));
    }
    if (const Result<void> bound = statement->bindText(1, "%" + phrase + "%"); !bound) {
        return std::unexpected{bound.error()};
    }

    std::vector<Hit> hits;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return hits;
        }
        const auto left = static_cast<float>(statement->real(column(TypedColumn::Left)));
        const auto top = static_cast<float>(statement->real(column(TypedColumn::Top)));
        const auto width = static_cast<float>(statement->real(column(TypedColumn::Width)));
        const auto height = static_cast<float>(statement->real(column(TypedColumn::Height)));
        hits.push_back(Hit{
            .section = statement->integer(column(TypedColumn::Section)),
            .page = statement->integer(column(TypedColumn::Page)),
            .found =
                FoundWord{
                    .pageId = statement->id(column(TypedColumn::PageId)),
                    .word =
                        InkWord{
                            .text = statement->text(column(TypedColumn::Text)),
                            .box =
                                Rect{
                                    .left = left,
                                    .top = top,
                                    .right = left + width,
                                    .bottom = top + height,
                                },
                            .strokes = {},
                        },
                },
        });
    }
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

[[nodiscard]] Result<std::vector<std::string>> columnsOf(sqlite3* database,
                                                         std::string_view table) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(database, "PRAGMA table_info(" + std::string{table} + ");");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    std::vector<std::string> names;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return names;
        }
        names.push_back(statement->text(1));
    }
}

[[nodiscard]] Result<void> ensurePaperColumns(sqlite3* database) {
    const Result<std::vector<std::string>> present = columnsOf(database, "pages");
    if (!present) {
        return std::unexpected{present.error()};
    }
    for (const auto& [name, kind] : kPaperColumns) {
        if (std::ranges::find(*present, name) != present->end()) {
            continue;
        }
        const std::string sql =
            "ALTER TABLE pages ADD COLUMN " + std::string{name} + " " + std::string{kind} + ";";
        if (const Result<void> added = sqlite::execute(database, sql); !added) {
            return added;
        }
    }
    return {};
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
    if (const Result<void> papered = ensurePaperColumns(database); !papered) {
        return papered;
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
    if (const Result<void> inserted = statement->run(); !inserted) {
        return inserted;
    }
    return touchInk(pageId);
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
    return touchInk(pageId);
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
    const auto gone = static_cast<std::size_t>(sqlite::changes(m_database));
    if (const Result<void> touched = touchInk(pageId); !touched) {
        return std::unexpected{touched.error()};
    }
    return gone;
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

Result<void> NotebookStore::touchInk(const Uuid& pageId) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "UPDATE pages SET ink_revision = ink_revision + 1 WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return bound;
    }
    return statement->run();
}

Result<std::int64_t> NotebookStore::inkRevisionOfPage(const Uuid& pageId) const {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "SELECT ink_revision FROM pages WHERE id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return std::unexpected{bound.error()};
    }
    const Result<bool> row = statement->step();
    if (!row) {
        return std::unexpected{row.error()};
    }
    if (!*row) {
        return makeError(ErrorCode::NotFound, "no such page");
    }
    return statement->integer(0);
}

Result<std::vector<Uuid>> NotebookStore::pagesWaitingToBeRead() const {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "SELECT pages.id FROM pages JOIN sections ON sections.id = pages.section_id "
                    "WHERE pages.trashed = 0 AND sections.trashed = 0 "
                    "  AND pages.read_revision != pages.ink_revision "
                    "ORDER BY sections.ordinal, pages.ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    std::vector<Uuid> pages;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return pages;
        }
        pages.push_back(statement->id(0));
    }
}

Result<void> NotebookStore::setWordsOfPage(const Uuid& pageId, std::int64_t inkRevision,
                                           std::span<const InkWord> words) {
    Result<sqlite::Transaction> transaction = sqlite::Transaction::begin(m_database);
    if (!transaction) {
        return std::unexpected{transaction.error()};
    }

    Result<sqlite::Statement> clear =
        sqlite::Statement::prepare(m_database, "DELETE FROM page_words WHERE page_id = ?;");
    if (!clear) {
        return std::unexpected{clear.error()};
    }
    if (const Result<void> bound = clear->bindId(1, pageId); !bound) {
        return bound;
    }
    if (const Result<void> cleared = clear->run(); !cleared) {
        return cleared;
    }

    Result<sqlite::Statement> insert = sqlite::Statement::prepare(
        m_database, "INSERT INTO page_words (page_id, ordinal, text, folded, left_edge, top_edge, "
                    "right_edge, bottom_edge, strokes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (!insert) {
        return std::unexpected{insert.error()};
    }
    std::int64_t ordinal = 0;
    for (const InkWord& word : words) {
        std::vector<std::byte> strokes;
        strokes.reserve(word.strokes.size() * Uuid::kByteCount);
        for (const Uuid& stroke : word.strokes) {
            for (const std::uint8_t byte : stroke.bytes()) {
                strokes.push_back(static_cast<std::byte>(byte));
            }
        }
        const Result<void> bound = bindAll({
            insert->bindId(1, pageId),
            insert->bindInteger(2, ordinal),
            insert->bindText(3, word.text),
            insert->bindText(4, folded(word.text)),
            insert->bindReal(5, word.box.left),
            insert->bindReal(6, word.box.top),
            insert->bindReal(7, word.box.right),
            insert->bindReal(8, word.box.bottom),
            insert->bindBlob(9, strokes),
        });
        if (!bound) {
            return bound;
        }
        if (const Result<void> written = insert->run(); !written) {
            return written;
        }
        if (const Result<void> ready = insert->reset(); !ready) {
            return ready;
        }
        ++ordinal;
    }

    Result<sqlite::Statement> read =
        sqlite::Statement::prepare(m_database, "UPDATE pages SET read_revision = ? WHERE id = ?;");
    if (!read) {
        return std::unexpected{read.error()};
    }
    if (const Result<void> bound =
            bindAll({read->bindInteger(1, inkRevision), read->bindId(2, pageId)});
        !bound) {
        return bound;
    }
    if (const Result<void> written = read->run(); !written) {
        return written;
    }
    return transaction->commit();
}

Result<std::vector<InkWord>> NotebookStore::wordsOfPage(const Uuid& pageId) const {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "SELECT text, left_edge, top_edge, right_edge, bottom_edge, strokes "
                    "FROM page_words WHERE page_id = ? ORDER BY ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return std::unexpected{bound.error()};
    }
    std::vector<InkWord> words;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return words;
        }
        words.push_back(wordFrom(*statement, 0));
    }
}

Result<std::vector<FoundWord>> NotebookStore::findWords(std::string_view text) const {
    const std::vector<std::string> wanted = foldedWords(text);
    if (wanted.empty()) {
        return std::vector<FoundWord>{};
    }

    Result<std::vector<Hit>> hits = writtenHits(m_database, *this, wanted);
    if (!hits) {
        return std::unexpected{hits.error()};
    }
    Result<std::vector<Hit>> typed = typedHits(m_database, wanted);
    if (!typed) {
        return std::unexpected{typed.error()};
    }
    hits->insert(hits->end(), std::make_move_iterator(typed->begin()),
                 std::make_move_iterator(typed->end()));
    std::ranges::stable_sort(*hits, {},
                             [](const Hit& hit) { return std::pair{hit.section, hit.page}; });

    std::vector<FoundWord> found;
    found.reserve(hits->size());
    for (Hit& hit : *hits) {
        found.push_back(std::move(hit.found));
    }
    return found;
}

Result<void> NotebookStore::insertText(const Uuid& pageId, const PlacedText& placed) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database,
        "INSERT INTO page_texts (id, page_id, ordinal, left_edge, top_edge, width, height, text, "
        "folded, font, size, color, align, line_height, marks) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    const TextBox& box = placed.box;
    const Result<void> bound = bindAll({
        statement->bindId(1, box.id),
        statement->bindId(2, pageId),
        statement->bindInteger(3, placed.ordinal),
        statement->bindReal(4, box.at.x),
        statement->bindReal(5, box.at.y),
        statement->bindReal(6, box.width),
        statement->bindReal(7, box.height),
        statement->bindText(8, box.text),
        statement->bindText(9, foldedRun(box.text)),
        statement->bindText(10, box.style.font),
        statement->bindReal(11, box.style.size),
        statement->bindInteger(12, packed(box.style.color)),
        statement->bindInteger(13, static_cast<std::int64_t>(box.style.align)),
        statement->bindReal(14, box.style.lineHeight),
        statement->bindInteger(15, marksOf(box.style)),
    });
    if (!bound) {
        return bound;
    }
    return statement->run();
}

Result<void> NotebookStore::updateText(const Uuid& pageId, const TextBox& box) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "UPDATE page_texts SET left_edge = ?, top_edge = ?, width = ?, height = ?, "
                    "text = ?, folded = ?, font = ?, size = ?, color = ?, align = ?, "
                    "line_height = ?, marks = ? WHERE page_id = ? AND id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    const Result<void> bound = bindAll({
        statement->bindReal(1, box.at.x),
        statement->bindReal(2, box.at.y),
        statement->bindReal(3, box.width),
        statement->bindReal(4, box.height),
        statement->bindText(5, box.text),
        statement->bindText(6, foldedRun(box.text)),
        statement->bindText(7, box.style.font),
        statement->bindReal(8, box.style.size),
        statement->bindInteger(9, packed(box.style.color)),
        statement->bindInteger(10, static_cast<std::int64_t>(box.style.align)),
        statement->bindReal(11, box.style.lineHeight),
        statement->bindInteger(12, marksOf(box.style)),
        statement->bindId(13, pageId),
        statement->bindId(14, box.id),
    });
    if (!bound) {
        return bound;
    }
    return statement->run();
}

Result<void> NotebookStore::removeText(const Uuid& pageId, const Uuid& textId) {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "DELETE FROM page_texts WHERE page_id = ? AND id = ?;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    const Result<void> bound = bindAll({
        statement->bindId(1, pageId),
        statement->bindId(2, textId),
    });
    if (!bound) {
        return bound;
    }
    return statement->run();
}

Result<std::size_t> NotebookStore::removeTextsOfPage(const Uuid& pageId) {
    Result<sqlite::Statement> statement =
        sqlite::Statement::prepare(m_database, "DELETE FROM page_texts WHERE page_id = ?;");
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

Result<std::vector<PlacedText>> NotebookStore::textsOfPage(const Uuid& pageId) const {
    Result<sqlite::Statement> statement = sqlite::Statement::prepare(
        m_database, "SELECT id, ordinal, left_edge, top_edge, width, height, text, font, size, "
                    "color, align, line_height, marks "
                    "FROM page_texts WHERE page_id = ? ORDER BY ordinal;");
    if (!statement) {
        return std::unexpected{statement.error()};
    }
    if (const Result<void> bound = statement->bindId(1, pageId); !bound) {
        return std::unexpected{bound.error()};
    }
    std::vector<PlacedText> texts;
    while (true) {
        const Result<bool> row = statement->step();
        if (!row) {
            return std::unexpected{row.error()};
        }
        if (!*row) {
            return texts;
        }
        texts.push_back(textFrom(*statement));
    }
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
        "DELETE FROM page_words WHERE page_id IN (SELECT id FROM pages WHERE trashed = 1 "
        "  OR section_id IN (SELECT id FROM sections WHERE trashed = 1));"
        "DELETE FROM page_texts WHERE page_id IN (SELECT id FROM pages WHERE trashed = 1 "
        "  OR section_id IN (SELECT id FROM sections WHERE trashed = 1));"
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
