#include "reflection/StdReflectionIntrospection.h"

#include "EmailDeliveryTaskCodec.h"
#include "auth/ChatLoginTicket.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace
{

struct LocalRouteRequestDto
{
    int uid = 0;
    std::string session_id;
    bool include_history = false;
};

#if MEMOCHAT_ENABLE_CPP26_REFLECTION

static_assert(memochat::reflection::FieldCount<LocalRouteRequestDto>() == 3);
static_assert(memochat::reflection::FieldName<LocalRouteRequestDto, 0>() == "uid");
static_assert(memochat::reflection::FieldName<LocalRouteRequestDto, 1>() == "session_id");
static_assert(memochat::reflection::FieldName<LocalRouteRequestDto, 2>() == "include_history");

static_assert(memochat::reflection::FieldNamesEqual<LocalRouteRequestDto>(
    std::array<std::string_view, 3>{"uid", "session_id", "include_history"}));

static_assert(memochat::reflection::FieldNamesEqual<varifyservice::EmailDeliveryTaskPayload>(
    std::array<std::string_view, 4>{"email", "code", "trace_id", "retry_count"}));

static_assert(memochat::reflection::FieldNamesEqual<memochat::auth::ChatLoginTicketClaims>(
    std::array<std::string_view, 12>{"uid",
                                     "user_id",
                                     "name",
                                     "nick",
                                     "icon",
                                     "desc",
                                     "email",
                                     "sex",
                                     "target_server",
                                     "protocol_version",
                                     "issued_at_ms",
                                     "expire_at_ms"}));

#endif

} // namespace

TEST(StdReflectionIntrospectionTest, ListsLocalDtoFieldsAtCompileTime)
{
#if MEMOCHAT_ENABLE_CPP26_REFLECTION
    constexpr auto names = memochat::reflection::FieldNames<LocalRouteRequestDto>();

    ASSERT_EQ(names.size(), 3U);
    EXPECT_EQ(names[0], "uid");
    EXPECT_EQ(names[1], "session_id");
    EXPECT_EQ(names[2], "include_history");
#else
    GTEST_SKIP() << "C++26 reflection is disabled for this target";
#endif
}

TEST(StdReflectionIntrospectionTest, ListsGlazeSerializedPilotDtoFields)
{
#if MEMOCHAT_ENABLE_CPP26_REFLECTION
    constexpr auto names = memochat::reflection::FieldNames<varifyservice::EmailDeliveryTaskPayload>();

    ASSERT_EQ(names.size(), 4U);
    EXPECT_EQ(names[0], "email");
    EXPECT_EQ(names[1], "code");
    EXPECT_EQ(names[2], "trace_id");
    EXPECT_EQ(names[3], "retry_count");
#else
    GTEST_SKIP() << "C++26 reflection is disabled for this target";
#endif
}

TEST(StdReflectionIntrospectionTest, ListsLoginTicketClaimFieldsWithoutOwningJson)
{
#if MEMOCHAT_ENABLE_CPP26_REFLECTION
    constexpr auto names = memochat::reflection::FieldNames<memochat::auth::ChatLoginTicketClaims>();

    ASSERT_EQ(names.size(), 12U);
    EXPECT_EQ(names[0], "uid");
    EXPECT_EQ(names[1], "user_id");
    EXPECT_EQ(names[8], "target_server");
    EXPECT_EQ(names[11], "expire_at_ms");
#else
    GTEST_SKIP() << "C++26 reflection is disabled for this target";
#endif
}
