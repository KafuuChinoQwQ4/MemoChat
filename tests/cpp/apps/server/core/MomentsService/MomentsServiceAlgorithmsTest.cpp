#include <gtest/gtest.h>

namespace memochat::tests::moments::service
{
const char* DefaultRelationQueryEndpoint();
bool ShouldUseDefaultRelationEndpoint(bool endpoint_empty);
int PublicVisibility();
int FriendsOnlyVisibility();
bool HasValidViewerAndAuthor(int viewer_uid, int author_uid);
bool CanViewWithoutRelationCheck(int viewer_uid, int author_uid, int visibility);
bool ShouldRejectNonFriendsVisibility(int visibility);
bool IsFriendsOnlyForeignMoment(int visibility, int viewer_uid, int author_uid);
int SuccessHttpStatus();
const char* JsonContentType();
const char* UidField();
const char* LoginTicketField();
bool HasRequiredAuthFields(bool has_uid, bool has_login_ticket);
bool HasValidUid(int uid);
bool HasValidMomentId(long long moment_id);
bool HasValidCommentId(long long comment_id);
bool HasValidNewComment(long long moment_id, bool content_empty);
int CommentLikeFetchLimit();
int DetailLikeFetchLimit();
int DetailCommentFetchLimit();
} // namespace memochat::tests::moments::service

TEST(MomentsServiceAlgorithmsTest, ExposesRelationEndpointAndVisibilityDecisions)
{
    using namespace memochat::tests::moments::service;

    EXPECT_STREQ(DefaultRelationQueryEndpoint(), "127.0.0.1:50090");
    EXPECT_TRUE(ShouldUseDefaultRelationEndpoint(true));
    EXPECT_FALSE(ShouldUseDefaultRelationEndpoint(false));
    EXPECT_EQ(PublicVisibility(), 0);
    EXPECT_EQ(FriendsOnlyVisibility(), 1);

    EXPECT_FALSE(HasValidViewerAndAuthor(0, 1));
    EXPECT_FALSE(HasValidViewerAndAuthor(1, 0));
    EXPECT_TRUE(HasValidViewerAndAuthor(1, 2));
    EXPECT_TRUE(CanViewWithoutRelationCheck(7, 7, FriendsOnlyVisibility()));
    EXPECT_TRUE(CanViewWithoutRelationCheck(7, 9, PublicVisibility()));
    EXPECT_FALSE(CanViewWithoutRelationCheck(7, 9, FriendsOnlyVisibility()));
    EXPECT_FALSE(ShouldRejectNonFriendsVisibility(FriendsOnlyVisibility()));
    EXPECT_TRUE(ShouldRejectNonFriendsVisibility(2));
    EXPECT_TRUE(IsFriendsOnlyForeignMoment(FriendsOnlyVisibility(), 7, 9));
    EXPECT_FALSE(IsFriendsOnlyForeignMoment(FriendsOnlyVisibility(), 7, 7));
    EXPECT_FALSE(IsFriendsOnlyForeignMoment(PublicVisibility(), 7, 9));
}

TEST(MomentsServiceAlgorithmsTest, ExposesResponseAuthAndIdGuards)
{
    using namespace memochat::tests::moments::service;

    EXPECT_EQ(SuccessHttpStatus(), 200);
    EXPECT_STREQ(JsonContentType(), "application/json");
    EXPECT_STREQ(UidField(), "uid");
    EXPECT_STREQ(LoginTicketField(), "login_ticket");
    EXPECT_TRUE(HasRequiredAuthFields(true, true));
    EXPECT_FALSE(HasRequiredAuthFields(true, false));
    EXPECT_FALSE(HasRequiredAuthFields(false, true));
    EXPECT_FALSE(HasValidUid(0));
    EXPECT_TRUE(HasValidUid(1));
    EXPECT_FALSE(HasValidMomentId(0));
    EXPECT_TRUE(HasValidMomentId(1));
    EXPECT_FALSE(HasValidCommentId(0));
    EXPECT_TRUE(HasValidCommentId(1));
    EXPECT_TRUE(HasValidNewComment(1, false));
    EXPECT_FALSE(HasValidNewComment(0, false));
    EXPECT_FALSE(HasValidNewComment(1, true));
}

TEST(MomentsServiceAlgorithmsTest, ExposesStableFetchLimits)
{
    using namespace memochat::tests::moments::service;

    EXPECT_EQ(CommentLikeFetchLimit(), 100);
    EXPECT_EQ(DetailLikeFetchLimit(), 100);
    EXPECT_EQ(DetailCommentFetchLimit(), 100);
}
