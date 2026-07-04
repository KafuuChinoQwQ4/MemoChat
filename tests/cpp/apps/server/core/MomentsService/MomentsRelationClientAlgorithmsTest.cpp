#include <gtest/gtest.h>

namespace memochat::tests::moments::relation_client
{
const char* ViewerUidField();
const char* AuthorUidsField();
const char* FriendUidsField();
const char* CompactIndentation();
int RpcDeadlineSeconds();
bool ShouldSkipFilter(bool endpoint_empty, int viewer_uid, bool author_uids_empty);
bool IsEnabled(bool endpoint_empty);
} // namespace memochat::tests::moments::relation_client

TEST(MomentsRelationClientAlgorithmsTest, ExposesRequestFieldNames)
{
    using namespace memochat::tests::moments::relation_client;

    EXPECT_STREQ(ViewerUidField(), "viewer_uid");
    EXPECT_STREQ(AuthorUidsField(), "author_uids");
    EXPECT_STREQ(FriendUidsField(), "friend_uids");
    EXPECT_STREQ(CompactIndentation(), "");
}

TEST(MomentsRelationClientAlgorithmsTest, ExposesRpcDeadline)
{
    using namespace memochat::tests::moments::relation_client;

    EXPECT_EQ(RpcDeadlineSeconds(), 2);
}

TEST(MomentsRelationClientAlgorithmsTest, SkipsFilterWhenFailClosedConditionsHold)
{
    using namespace memochat::tests::moments::relation_client;

    // Endpoint unset -> skip regardless of other inputs.
    EXPECT_TRUE(ShouldSkipFilter(true, 7, false));
    // Non-positive viewer uid -> skip.
    EXPECT_TRUE(ShouldSkipFilter(false, 0, false));
    EXPECT_TRUE(ShouldSkipFilter(false, -1, false));
    // Empty author uid list -> skip.
    EXPECT_TRUE(ShouldSkipFilter(false, 7, true));
    // All valid -> run the filter.
    EXPECT_FALSE(ShouldSkipFilter(false, 7, false));
}

TEST(MomentsRelationClientAlgorithmsTest, EnabledMirrorsEndpointPresence)
{
    using namespace memochat::tests::moments::relation_client;

    EXPECT_FALSE(IsEnabled(true));
    EXPECT_TRUE(IsEnabled(false));
}
