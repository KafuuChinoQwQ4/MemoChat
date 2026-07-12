#pragma once

#include <libpq-fe.h>

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace memo::db
{
namespace detail
{
struct QueryState
{
    bool ok = true;
    std::string error;

    void Fail(std::string message)
    {
        if (ok)
        {
            ok = false;
            error = std::move(message);
        }
    }
};

inline std::string TrimPostgresMessage(const char* message)
{
    std::string value = message == nullptr ? std::string{} : std::string(message);
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
    {
        value.pop_back();
    }
    return value;
}

template <typename T> std::string ParamText(const T& value)
{
    using Value = std::remove_cvref_t<T>;
    if constexpr (std::same_as<Value, std::string>)
    {
        return value;
    }
    else if constexpr (std::same_as<Value, std::string_view>)
    {
        return std::string(value);
    }
    else if constexpr (std::same_as<Value, const char*> || std::same_as<Value, char*>)
    {
        return value == nullptr ? std::string{} : std::string(value);
    }
    else if constexpr (std::is_array_v<Value> && std::same_as<std::remove_cv_t<std::remove_extent_t<Value>>, char>)
    {
        return std::string(value);
    }
    else if constexpr (std::same_as<Value, bool>)
    {
        return value ? "true" : "false";
    }
    else if constexpr (std::is_enum_v<Value>)
    {
        return ParamText(static_cast<std::underlying_type_t<Value>>(value));
    }
    else if constexpr (std::integral<Value>)
    {
        char buffer[std::numeric_limits<Value>::digits10 + 4]{};
        const auto converted = std::to_chars(std::begin(buffer), std::end(buffer), value);
        return converted.ec == std::errc{} ? std::string(buffer, converted.ptr) : std::string{};
    }
    else if constexpr (std::floating_point<Value>)
    {
        char buffer[128]{};
        const auto converted = std::to_chars(std::begin(buffer), std::end(buffer), value);
        return converted.ec == std::errc{} ? std::string(buffer, converted.ptr) : std::string{};
    }
    else
    {
        static_assert(std::is_same_v<Value, void>, "Unsupported PostgreSQL parameter type");
    }
}
} // namespace detail

class params
{
public:
    params() = default;

    template <typename... Args>
        requires(sizeof...(Args) > 0)
    explicit params(Args&&... args)
    {
        values_.reserve(sizeof...(Args));
        (append(std::forward<Args>(args)), ...);
    }

    template <typename T> void append(T&& value)
    {
        values_.push_back(detail::ParamText(std::forward<T>(value)));
    }

    const std::vector<std::string>& values() const noexcept
    {
        return values_;
    }

private:
    std::vector<std::string> values_;
};

class field
{
public:
    field() = default;

    bool is_null() const noexcept
    {
        return result_ == nullptr || row_ < 0 || column_ < 0 || PQgetisnull(result_.get(), row_, column_) != 0;
    }

    const char* c_str() const noexcept
    {
        if (is_null())
        {
            return "";
        }
        return PQgetvalue(result_.get(), row_, column_);
    }

    template <typename T> T as() const
    {
        using Value = std::remove_cvref_t<T>;
        if (is_null())
        {
            Fail("PostgreSQL field is null");
            return Value{};
        }

        const std::string_view text(c_str(), static_cast<std::size_t>(PQgetlength(result_.get(), row_, column_)));
        if constexpr (std::same_as<Value, std::string>)
        {
            return std::string(text);
        }
        else if constexpr (std::same_as<Value, bool>)
        {
            if (text == "t" || text == "true" || text == "1")
            {
                return true;
            }
            if (text == "f" || text == "false" || text == "0")
            {
                return false;
            }
            Fail("PostgreSQL boolean conversion failed");
            return false;
        }
        else if constexpr (std::integral<Value> || std::floating_point<Value>)
        {
            Value converted{};
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), converted);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
            {
                Fail("PostgreSQL numeric conversion failed");
                return Value{};
            }
            return converted;
        }
        else
        {
            static_assert(std::is_same_v<Value, void>, "Unsupported PostgreSQL field conversion");
        }
    }

private:
    friend class row;

    field(std::shared_ptr<PGresult> result, int row, int column, std::shared_ptr<detail::QueryState> state)
        : result_(std::move(result))
        , state_(std::move(state))
        , row_(row)
        , column_(column)
    {
    }

    void Fail(std::string message) const
    {
        if (state_ != nullptr)
        {
            state_->Fail(std::move(message));
        }
    }

    std::shared_ptr<PGresult> result_;
    std::shared_ptr<detail::QueryState> state_;
    int row_ = -1;
    int column_ = -1;
};

class row
{
public:
    row() = default;

    field operator[](std::size_t column) const
    {
        if (result_ == nullptr || row_ < 0 || column >= size())
        {
            Fail("PostgreSQL column index is out of range");
            return field{};
        }
        return field(result_, row_, static_cast<int>(column), state_);
    }

    field operator[](std::string_view name) const
    {
        if (result_ == nullptr || row_ < 0)
        {
            Fail("PostgreSQL row is invalid");
            return field{};
        }
        const std::string owned_name(name);
        const int column = PQfnumber(result_.get(), owned_name.c_str());
        if (column < 0)
        {
            Fail("PostgreSQL column does not exist: " + owned_name);
            return field{};
        }
        return field(result_, row_, column, state_);
    }

    std::size_t size() const noexcept
    {
        return result_ == nullptr ? 0U : static_cast<std::size_t>(PQnfields(result_.get()));
    }

    explicit operator bool() const noexcept
    {
        return result_ != nullptr && row_ >= 0;
    }

private:
    friend class result;

    row(std::shared_ptr<PGresult> result, int row_index, std::shared_ptr<detail::QueryState> state)
        : result_(std::move(result))
        , state_(std::move(state))
        , row_(row_index)
    {
    }

    void Fail(std::string message) const
    {
        if (state_ != nullptr)
        {
            state_->Fail(std::move(message));
        }
    }

    std::shared_ptr<PGresult> result_;
    std::shared_ptr<detail::QueryState> state_;
    int row_ = -1;
};

class result
{
public:
    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = row;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        row operator*() const
        {
            return row(result_, row_, state_);
        }

        iterator& operator++()
        {
            ++row_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++(*this);
            return old;
        }

        bool operator==(const iterator&) const = default;

    private:
        friend class result;

        iterator(std::shared_ptr<PGresult> result, int row_index, std::shared_ptr<detail::QueryState> state)
            : result_(std::move(result))
            , state_(std::move(state))
            , row_(row_index)
        {
        }

        std::shared_ptr<PGresult> result_;
        std::shared_ptr<detail::QueryState> state_;
        int row_ = 0;
    };

    result()
        : state_(std::make_shared<detail::QueryState>())
    {
        state_->Fail("PostgreSQL result is not initialized");
    }

    bool ok() const noexcept
    {
        return state_ != nullptr && state_->ok;
    }

    const std::string& error_message() const noexcept
    {
        static const std::string empty;
        return state_ == nullptr ? empty : state_->error;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    std::size_t size() const noexcept
    {
        return result_ == nullptr ? 0U : static_cast<std::size_t>(PQntuples(result_.get()));
    }

    row operator[](std::size_t index) const
    {
        if (index >= size())
        {
            state_->Fail("PostgreSQL row index is out of range");
            return row{};
        }
        return row(result_, static_cast<int>(index), state_);
    }

    std::size_t affected_rows() const noexcept
    {
        if (!ok() || result_ == nullptr)
        {
            return 0;
        }
        const char* text = PQcmdTuples(result_.get());
        if (text == nullptr || *text == '\0')
        {
            return 0;
        }
        std::size_t rows = 0;
        const std::string_view value(text);
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), rows);
        return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() ? rows : 0;
    }

    result no_rows() const
    {
        if (ok() && !empty())
        {
            state_->Fail("PostgreSQL command unexpectedly returned rows");
        }
        return *this;
    }

    row one_row() const
    {
        if (!ok() || size() != 1)
        {
            state_->Fail("PostgreSQL query did not return exactly one row");
            return row{};
        }
        return (*this)[0];
    }

    iterator begin() const
    {
        return iterator(result_, 0, state_);
    }

    iterator end() const
    {
        return iterator(result_, static_cast<int>(size()), state_);
    }

private:
    friend class connection;
    friend class transaction;

    struct ResultDeleter
    {
        void operator()(PGresult* query_result) const noexcept
        {
            if (query_result != nullptr)
            {
                PQclear(query_result);
            }
        }
    };

    result(PGresult* raw_result, std::shared_ptr<detail::QueryState> state)
        : result_(raw_result, ResultDeleter{})
        , state_(std::move(state))
    {
    }

    std::shared_ptr<PGresult> result_;
    std::shared_ptr<detail::QueryState> state_;
};

class connection
{
public:
    explicit connection(std::string_view connection_string)
    {
        if (connection_string.empty())
        {
            error_ = "PostgreSQL connection string is empty";
            return;
        }
        const std::string owned_connection_string(connection_string);
        connection_.reset(PQconnectdb(owned_connection_string.c_str()));
        if (connection_ == nullptr)
        {
            error_ = "PQconnectdb returned null";
        }
        else if (PQstatus(connection_.get()) != CONNECTION_OK)
        {
            error_ = detail::TrimPostgresMessage(PQerrorMessage(connection_.get()));
        }
    }

    bool is_open() const noexcept
    {
        return connection_ != nullptr && PQstatus(connection_.get()) == CONNECTION_OK;
    }

    const std::string& error_message() const noexcept
    {
        return error_;
    }

private:
    friend class transaction;

    struct ConnectionDeleter
    {
        void operator()(PGconn* connection) const noexcept
        {
            if (connection != nullptr)
            {
                PQfinish(connection);
            }
        }
    };

    result Execute(std::string_view query, const params* query_params, const std::shared_ptr<detail::QueryState>& state)
    {
        if (!is_open())
        {
            state->Fail(error_.empty() ? "PostgreSQL connection is not open" : error_);
            return result(nullptr, state);
        }

        const std::string owned_query(query);
        PGresult* raw_result = nullptr;
        if (query_params == nullptr)
        {
            raw_result = PQexec(connection_.get(), owned_query.c_str());
        }
        else
        {
            std::vector<const char*> values;
            values.reserve(query_params->values().size());
            for (const auto& value : query_params->values())
            {
                values.push_back(value.c_str());
            }
            raw_result = PQexecParams(connection_.get(),
                                      owned_query.c_str(),
                                      static_cast<int>(values.size()),
                                      nullptr,
                                      values.data(),
                                      nullptr,
                                      nullptr,
                                      0);
        }

        if (raw_result == nullptr)
        {
            state->Fail(detail::TrimPostgresMessage(PQerrorMessage(connection_.get())));
            return result(nullptr, state);
        }

        const ExecStatusType status = PQresultStatus(raw_result);
        if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
        {
            state->Fail(detail::TrimPostgresMessage(PQresultErrorMessage(raw_result)));
        }
        return result(raw_result, state);
    }

    std::unique_ptr<PGconn, ConnectionDeleter> connection_;
    std::string error_;
};

class transaction
{
public:
    explicit transaction(connection& database, bool read_only = false)
        : database_(&database)
        , state_(std::make_shared<detail::QueryState>())
    {
        if (!database.is_open())
        {
            state_->Fail(database.error_message().empty() ? "PostgreSQL connection is not open"
                                                          : database.error_message());
            return;
        }
        const result started = database_->Execute(read_only ? "BEGIN READ ONLY" : "BEGIN", nullptr, state_);
        active_ = started.ok();
    }

    transaction(const transaction&) = delete;
    transaction& operator=(const transaction&) = delete;

    ~transaction()
    {
        if (active_ && database_ != nullptr)
        {
            const auto rollback_state = std::make_shared<detail::QueryState>();
            database_->Execute("ROLLBACK", nullptr, rollback_state);
        }
    }

    result exec(std::string_view query)
    {
        if (!active_)
        {
            state_->Fail("PostgreSQL transaction is not active");
            return result(nullptr, state_);
        }
        if (!ok())
        {
            return result(nullptr, state_);
        }
        return database_->Execute(query, nullptr, state_);
    }

    result exec(std::string_view query, const params& query_params)
    {
        if (!active_)
        {
            state_->Fail("PostgreSQL transaction is not active");
            return result(nullptr, state_);
        }
        if (!ok())
        {
            return result(nullptr, state_);
        }
        return database_->Execute(query, &query_params, state_);
    }

    template <typename... Args> result exec_params(std::string_view query, Args&&... args)
    {
        return exec(query, params(std::forward<Args>(args)...));
    }

    result exec0(std::string_view query)
    {
        return exec(query).no_rows();
    }

    template <typename... Args> result exec_params0(std::string_view query, Args&&... args)
    {
        return exec_params(query, std::forward<Args>(args)...).no_rows();
    }

    row exec1(std::string_view query)
    {
        return exec(query).one_row();
    }

    bool commit()
    {
        if (!active_ || !ok())
        {
            abort();
            return false;
        }
        const result committed = database_->Execute("COMMIT", nullptr, state_);
        active_ = !committed.ok();
        return committed.ok();
    }

    void abort()
    {
        if (!active_ || database_ == nullptr)
        {
            return;
        }
        const auto rollback_state = std::make_shared<detail::QueryState>();
        database_->Execute("ROLLBACK", nullptr, rollback_state);
        active_ = false;
    }

    bool ok() const noexcept
    {
        return state_ != nullptr && state_->ok;
    }

    const std::string& error_message() const noexcept
    {
        static const std::string empty;
        return state_ == nullptr ? empty : state_->error;
    }

    std::string quote_name(std::string_view identifier)
    {
        if (!active_)
        {
            state_->Fail("PostgreSQL transaction is not active");
            return {};
        }
        if (!ok())
        {
            return {};
        }
        const std::string owned_identifier(identifier);
        char* escaped =
            PQescapeIdentifier(database_->connection_.get(), owned_identifier.data(), owned_identifier.size());
        if (escaped == nullptr)
        {
            state_->Fail(detail::TrimPostgresMessage(PQerrorMessage(database_->connection_.get())));
            return {};
        }
        std::string result_text(escaped);
        PQfreemem(escaped);
        return result_text;
    }

private:
    connection* database_ = nullptr;
    std::shared_ptr<detail::QueryState> state_;
    bool active_ = false;
};

class work final : public transaction
{
public:
    explicit work(connection& database)
        : transaction(database, false)
    {
    }
};

class read_transaction final : public transaction
{
public:
    explicit read_transaction(connection& database)
        : transaction(database, true)
    {
    }
};

template <typename Tx> result Exec0(Tx& tx, std::string_view query)
{
    return tx.exec0(query);
}

template <typename Tx> row Exec1(Tx& tx, std::string_view query)
{
    return tx.exec1(query);
}

template <typename Tx, typename... Args> result ExecParams(Tx& tx, std::string_view query, Args&&... args)
{
    return tx.exec_params(query, std::forward<Args>(args)...);
}

template <typename Tx, typename... Args> result ExecParams0(Tx& tx, std::string_view query, Args&&... args)
{
    return tx.exec_params0(query, std::forward<Args>(args)...);
}
} // namespace memo::db

namespace pqxx = memo::db;
