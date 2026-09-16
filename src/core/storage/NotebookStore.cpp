#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/storage/StrokeCodec.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

/// Statements that create the first version of the schema. Later versions append their own step
/// to this list and never edit an earlier one, so that an old file upgrades in order.
constexpr std::string_view kSchemaVersion1 = R"sql(
    CREATE TABLE strokes (
        id        BLOB PRIMARY KEY NOT NULL,
        page_id   BLOB NOT NULL,
        ordinal   INTEGER NOT NULL,
        data      BLOB NOT NULL
    );
    CREATE INDEX strokes_by_page ON strokes (page_id, ordinal);
)sql";

[[nodiscard]] Error sqliteError(sqlite3* database, std::string_view what) {
    std::string message{what};
    if (database != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(database);
    }
    return Error{.code = ErrorCode::IoFailure, .message = std::move(message)};
}

[[nodiscard]] Result<void> execute(sqlite3* database, std::string_view sql) {
    char* rawMessage = nullptr;
    const int status =
        sqlite3_exec(database, std::string{sql}.c_str(), nullptr, nullptr, &rawMessage);
    if (status != SQLITE_OK) {
        std::string message{"statement failed"};
        if (rawMessage != nullptr) {
            message += ": ";
            message += rawMessage;
            sqlite3_free(rawMessage);
        }
        return std::unexpected<Error>{Error{.code = ErrorCode::IoFailure, .message = message}};
    }
    return {};
}

/// Runs a statement that returns a single integer, such as a pragma.
[[nodiscard]] Result<int> queryInteger(sqlite3* database, std::string_view sql) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, std::string{sql}.c_str(), -1, &statement, nullptr)
        != SQLITE_OK) {
        return std::unexpected{sqliteError(database, "could not prepare a query")};
    }
    int value = 0;
    const int status = sqlite3_step(statement);
    if (status == SQLITE_ROW) {
        value = sqlite3_column_int(statement, 0);
    }
    sqlite3_finalize(statement);
    if (status != SQLITE_ROW && status != SQLITE_DONE) {
        return std::unexpected{sqliteError(database, "could not run a query")};
    }
    return value;
}

[[nodiscard]] Result<void> bindBlob(sqlite3_stmt* statement, int index,
                                    std::span<const std::byte> bytes) {
    const int status = sqlite3_bind_blob(statement, index, bytes.data(),
                                         static_cast<int>(bytes.size()), SQLITE_TRANSIENT);
    if (status != SQLITE_OK) {
        return std::unexpected<Error>{
            Error{.code = ErrorCode::IoFailure, .message = "could not bind a value"}};
    }
    return {};
}

[[nodiscard]] std::span<const std::byte> idBytes(const Uuid& id) {
    return std::as_bytes(std::span{id.bytes()});
}

/// Abandons a half applied migration and returns @p cause, the error that made it necessary.
[[nodiscard]] Result<void> rollbackWith(sqlite3* database, Error cause) {
    if (const Result<void> rolledBack = execute(database, "ROLLBACK;"); !rolledBack) {
        cause.message += " (the rollback failed as well: " + rolledBack.error().message + ")";
    }
    return std::unexpected{std::move(cause)};
}

/// Brings the schema of an open database up to kNotebookSchemaVersion.
[[nodiscard]] Result<void> migrate(sqlite3* database) {
    const Result<int> version = queryInteger(database, "PRAGMA user_version;");
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

    if (const Result<void> begun = execute(database, "BEGIN IMMEDIATE;"); !begun) {
        return begun;
    }
    if (*version < 1) {
        if (const Result<void> applied = execute(database, kSchemaVersion1); !applied) {
            return rollbackWith(database, applied.error());
        }
    }
    if (const Result<void> stamped = execute(
            database, "PRAGMA user_version = " + std::to_string(kNotebookSchemaVersion) + ";");
        !stamped) {
        return rollbackWith(database, stamped.error());
    }
    return execute(database, "COMMIT;");
}

} // namespace

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
    sqlite3* database = nullptr;
    const int status = sqlite3_open_v2(path.string().c_str(), &database,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (status != SQLITE_OK) {
        Error error = sqliteError(database, "could not open the notebook");
        sqlite3_close(database);
        return std::unexpected{std::move(error)};
    }

    NotebookStore store{database};

    // Readers never block the writer, and a finished stroke survives a crash of the application.
    for (const std::string_view pragma : {
             "PRAGMA journal_mode = WAL;",
             "PRAGMA foreign_keys = ON;",
             "PRAGMA synchronous = NORMAL;",
         }) {
        if (const Result<void> applied = execute(database, pragma); !applied) {
            return std::unexpected{applied.error()};
        }
    }

    if (const Result<void> migrated = migrate(database); !migrated) {
        return std::unexpected{migrated.error()};
    }
    return store;
}

Result<void> NotebookStore::appendStroke(const Uuid& pageId, const Stroke& stroke) {
    // The ordinal is chosen inside the statement, so that choosing it and inserting the row are
    // one atomic step.
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_database,
                           "INSERT INTO strokes (id, page_id, ordinal, data) VALUES (?, ?, "
                           "(SELECT COALESCE(MAX(ordinal) + 1, 0) FROM strokes WHERE page_id = ?),"
                           " ?);",
                           -1, &statement, nullptr)
        != SQLITE_OK) {
        return std::unexpected{sqliteError(m_database, "could not prepare the insert")};
    }

    const std::vector<std::byte> data = encodeStroke(stroke);
    const auto bindings = {
        bindBlob(statement, 1, idBytes(stroke.id())),
        bindBlob(statement, 2, idBytes(pageId)),
        bindBlob(statement, 3, idBytes(pageId)),
        bindBlob(statement, 4, data),
    };
    for (const Result<void>& binding : bindings) {
        if (!binding) {
            sqlite3_finalize(statement);
            return binding;
        }
    }

    const int status = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (status != SQLITE_DONE) {
        return std::unexpected{sqliteError(m_database, "could not store the stroke")};
    }
    return {};
}

Result<std::vector<Stroke>> NotebookStore::strokesOfPage(const Uuid& pageId) const {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_database,
                           "SELECT data FROM strokes WHERE page_id = ? ORDER BY ordinal;", -1,
                           &statement, nullptr)
        != SQLITE_OK) {
        return std::unexpected{sqliteError(m_database, "could not prepare the query")};
    }
    if (const Result<void> bound = bindBlob(statement, 1, idBytes(pageId)); !bound) {
        sqlite3_finalize(statement);
        return std::unexpected{bound.error()};
    }

    std::vector<Stroke> strokes;
    while (true) {
        const int status = sqlite3_step(statement);
        if (status == SQLITE_DONE) {
            break;
        }
        if (status != SQLITE_ROW) {
            sqlite3_finalize(statement);
            return std::unexpected{sqliteError(m_database, "could not read the strokes")};
        }

        const auto* const data = static_cast<const std::byte*>(sqlite3_column_blob(statement, 0));
        const auto size = static_cast<std::size_t>(sqlite3_column_bytes(statement, 0));
        Result<Stroke> stroke = decodeStroke(std::span{data, size});
        if (!stroke) {
            sqlite3_finalize(statement);
            return std::unexpected{stroke.error()};
        }
        strokes.push_back(std::move(*stroke));
    }
    sqlite3_finalize(statement);
    return strokes;
}

Result<int> NotebookStore::schemaVersion() const {
    return queryInteger(m_database, "PRAGMA user_version;");
}

} // namespace phvikapen::core
