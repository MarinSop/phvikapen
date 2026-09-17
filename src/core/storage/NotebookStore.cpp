#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/storage/Sqlite.hpp"
#include "core/storage/StrokeCodec.hpp"

#include <sqlite3.h>

#include <array>
#include <cstddef>
#include <cstdint>
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

constexpr std::array kMigrations{
    std::pair{1, kSchemaVersion1},
    std::pair{2, kSchemaVersion2},
};

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

Result<int> NotebookStore::schemaVersion() const {
    const Result<std::int64_t> version = queryInteger(m_database, "PRAGMA user_version;");
    if (!version) {
        return std::unexpected{version.error()};
    }
    return static_cast<int>(*version);
}

}
