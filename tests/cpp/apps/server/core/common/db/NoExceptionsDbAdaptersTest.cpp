#include <gtest/gtest.h>

#include "db/MongoCCompat.hpp"
#include "db/PqxxCompat.hpp"

#include <string>

TEST(NoExceptionsDbAdaptersTest, SerializesPostgresParametersWithoutExceptionChannels)
{
    pqxx::params values{"memo", 42, 9LL, true};
    values.append(std::string_view("chat"));

    ASSERT_EQ(values.values().size(), 5U);
    EXPECT_EQ(values.values()[0], "memo");
    EXPECT_EQ(values.values()[1], "42");
    EXPECT_EQ(values.values()[2], "9");
    EXPECT_EQ(values.values()[3], "true");
    EXPECT_EQ(values.values()[4], "chat");
}

TEST(NoExceptionsDbAdaptersTest, ReportsInvalidPostgresConnectionAsState)
{
    pqxx::connection empty_connection("");
    EXPECT_FALSE(empty_connection.is_open());
    EXPECT_EQ(empty_connection.error_message(), "PostgreSQL connection string is empty");

    pqxx::connection connection("not-a-postgres-connection-option=1");
    EXPECT_FALSE(connection.is_open());
    EXPECT_FALSE(connection.error_message().empty());

    pqxx::work transaction(connection);
    EXPECT_FALSE(transaction.ok());
    EXPECT_FALSE(transaction.exec("SELECT 1").ok());
    EXPECT_FALSE(transaction.commit());
    EXPECT_FALSE(transaction.error_message().empty());
}

TEST(NoExceptionsDbAdaptersTest, ReportsInvalidMongoUriAsState)
{
    memo::db::MongoClientPool pool;
    std::string error;

    EXPECT_FALSE(pool.Open("://invalid", error));
    EXPECT_FALSE(pool.IsOpen());
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(static_cast<bool>(pool.Acquire()));
}

TEST(NoExceptionsDbAdaptersTest, ClearsStaleMongoErrorWhenPoolOpens)
{
    memo::db::MongoClientPool pool;
    std::string error = "stale";

    EXPECT_TRUE(pool.Open("mongodb://127.0.0.1:27017", error));
    EXPECT_TRUE(pool.IsOpen());
    EXPECT_TRUE(error.empty());
}
