#include "core/storage/Sqlite.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace phvikapen::core::sqlite {

Error lastError(sqlite3* database, std::string_view what) {
    std::string message{what};
    if (database != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(database);
    }
    return Error{.code = ErrorCode::IoFailure, .message = std::move(message)};
}

Result<void> execute(sqlite3* database, std::string_view sql) {
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
        return makeError(ErrorCode::IoFailure, std::move(message));
    }
    return {};
}

std::int64_t changes(sqlite3* database) noexcept {
    return sqlite3_changes64(database);
}

Statement::Statement(sqlite3* database, sqlite3_stmt* statement) noexcept
    : m_database{database}, m_statement{statement} {}

Result<Statement> Statement::prepare(sqlite3* database, std::string_view sql) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, sql.data(), static_cast<int>(sql.size()), &statement, nullptr)
        != SQLITE_OK) {
        return std::unexpected{lastError(database, "could not prepare a statement")};
    }
    return Statement{database, statement};
}

Statement::~Statement() {
    sqlite3_finalize(m_statement);
}

Statement::Statement(Statement&& other) noexcept
    : m_database{other.m_database}, m_statement{std::exchange(other.m_statement, nullptr)} {}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        sqlite3_finalize(m_statement);
        m_database = other.m_database;
        m_statement = std::exchange(other.m_statement, nullptr);
    }
    return *this;
}

Result<void> Statement::checkBind(int status) const {
    if (status != SQLITE_OK) {
        return std::unexpected{lastError(m_database, "could not bind a value")};
    }
    return {};
}

Result<void> Statement::bindBlob(int index, std::span<const std::byte> bytes) {
    return checkBind(sqlite3_bind_blob(m_statement, index, bytes.data(),
                                       static_cast<int>(bytes.size()), SQLITE_TRANSIENT));
}

Result<void> Statement::bindId(int index, const Uuid& id) {
    return bindBlob(index, std::as_bytes(std::span{id.bytes()}));
}

Result<void> Statement::bindInteger(int index, std::int64_t value) {
    return checkBind(sqlite3_bind_int64(m_statement, index, value));
}

Result<void> Statement::bindReal(int index, double value) {
    return checkBind(sqlite3_bind_double(m_statement, index, value));
}

Result<void> Statement::bindText(int index, std::string_view text) {
    return checkBind(sqlite3_bind_text(m_statement, index, text.data(),
                                       static_cast<int>(text.size()), SQLITE_TRANSIENT));
}

Result<void> Statement::bindNull(int index) {
    return checkBind(sqlite3_bind_null(m_statement, index));
}

Result<bool> Statement::step() {
    const int status = sqlite3_step(m_statement);
    if (status == SQLITE_ROW) {
        return true;
    }
    if (status == SQLITE_DONE) {
        return false;
    }
    return std::unexpected{lastError(m_database, "could not run a statement")};
}

Result<void> Statement::run() {
    const Result<bool> stepped = step();
    if (!stepped) {
        return std::unexpected{stepped.error()};
    }
    return {};
}

Result<void> Statement::reset() {
    if (sqlite3_reset(m_statement) != SQLITE_OK) {
        return std::unexpected{lastError(m_database, "could not reset a statement")};
    }
    return {};
}

std::int64_t Statement::integer(int column) const noexcept {
    return sqlite3_column_int64(m_statement, column);
}

double Statement::real(int column) const noexcept {
    return sqlite3_column_double(m_statement, column);
}

std::string Statement::text(int column) const {
    const auto* const characters =
        static_cast<const char*>(sqlite3_column_blob(m_statement, column));
    const auto size = static_cast<std::size_t>(sqlite3_column_bytes(m_statement, column));
    return characters == nullptr ? std::string{} : std::string{characters, size};
}

std::span<const std::byte> Statement::blob(int column) const noexcept {
    const auto* const data =
        static_cast<const std::byte*>(sqlite3_column_blob(m_statement, column));
    const auto size = static_cast<std::size_t>(sqlite3_column_bytes(m_statement, column));
    return data == nullptr ? std::span<const std::byte>{} : std::span{data, size};
}

Uuid Statement::id(int column) const noexcept {
    Uuid::Bytes bytes{};
    const std::span<const std::byte> stored = blob(column);
    if (stored.size() == bytes.size()) {
        std::ranges::transform(stored, bytes.begin(),
                               [](std::byte value) { return static_cast<std::uint8_t>(value); });
    }
    return Uuid{bytes};
}

Transaction::Transaction(sqlite3* database) noexcept : m_database{database} {}

Result<Transaction> Transaction::begin(sqlite3* database) {
    if (const Result<void> begun = execute(database, "SAVEPOINT change;"); !begun) {
        return std::unexpected{begun.error()};
    }
    return Transaction{database};
}

Transaction::~Transaction() {
    if (m_database != nullptr) {
        sqlite3_exec(m_database, "ROLLBACK TO change; RELEASE change;", nullptr, nullptr, nullptr);
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : m_database{std::exchange(other.m_database, nullptr)} {}

Result<void> Transaction::commit() {
    if (const Result<void> committed = execute(m_database, "RELEASE change;"); !committed) {
        return committed;
    }
    m_database = nullptr;
    return {};
}

}
