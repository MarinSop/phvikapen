#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace phvikapen::core::sqlite {

[[nodiscard]] Error lastError(sqlite3* database, std::string_view what);

[[nodiscard]] Result<void> execute(sqlite3* database, std::string_view sql);

[[nodiscard]] std::int64_t changes(sqlite3* database) noexcept;

class Statement {
public:
    [[nodiscard]] static Result<Statement> prepare(sqlite3* database, std::string_view sql);

    ~Statement();

    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] Result<void> bindBlob(int index, std::span<const std::byte> bytes);
    [[nodiscard]] Result<void> bindId(int index, const Uuid& id);
    [[nodiscard]] Result<void> bindInteger(int index, std::int64_t value);
    [[nodiscard]] Result<void> bindReal(int index, double value);
    [[nodiscard]] Result<void> bindText(int index, std::string_view text);

    [[nodiscard]] Result<bool> step();
    [[nodiscard]] Result<void> run();
    [[nodiscard]] Result<void> reset();

    [[nodiscard]] std::int64_t integer(int column) const noexcept;
    [[nodiscard]] double real(int column) const noexcept;
    [[nodiscard]] std::string text(int column) const;
    [[nodiscard]] std::span<const std::byte> blob(int column) const noexcept;
    [[nodiscard]] Uuid id(int column) const noexcept;

private:
    Statement(sqlite3* database, sqlite3_stmt* statement) noexcept;

    [[nodiscard]] Result<void> checkBind(int status) const;

    sqlite3* m_database{nullptr};
    sqlite3_stmt* m_statement{nullptr};
};

class Transaction {
public:
    [[nodiscard]] static Result<Transaction> begin(sqlite3* database);

    ~Transaction();

    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) = delete;

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    [[nodiscard]] Result<void> commit();

private:
    explicit Transaction(sqlite3* database) noexcept;

    sqlite3* m_database{nullptr};
};

}
