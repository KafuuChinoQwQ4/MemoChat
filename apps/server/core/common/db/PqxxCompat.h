#pragma once

#include <pqxx/pqxx>

#include <utility>

namespace memo::db {

template <typename Tx>
pqxx::result Exec0(Tx& tx, std::string_view query)
{
	return tx.exec(query).no_rows();
}

template <typename Tx>
pqxx::row Exec1(Tx& tx, std::string_view query)
{
	return tx.exec(query).one_row();
}

template <typename Tx, typename... Args>
pqxx::result ExecParams(Tx& tx, std::string_view query, Args&&... args)
{
	return tx.exec(query, pqxx::params{std::forward<Args>(args)...});
}

template <typename Tx, typename... Args>
pqxx::result ExecParams0(Tx& tx, std::string_view query, Args&&... args)
{
	return tx.exec(query, pqxx::params{std::forward<Args>(args)...}).no_rows();
}

} // namespace memo::db

#ifndef MEMOCHAT_PQXX_COMPAT_NO_METHOD_MACROS
#define exec0(query) exec(query).no_rows()
#define exec1(query) exec(query).one_row()
#define exec_params(query, ...) exec(query, pqxx::params{__VA_ARGS__})
#define exec_params0(query, ...) exec(query, pqxx::params{__VA_ARGS__}).no_rows()
#endif
