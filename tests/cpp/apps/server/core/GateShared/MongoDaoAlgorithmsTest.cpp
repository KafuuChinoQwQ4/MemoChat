#include <gtest/gtest.h>

namespace memochat::tests::gate::mongo_dao
{
bool ParseBoolText(const char* text);
const char* DefaultMomentsCollection();
bool IsEnabled(bool enabled, bool init_ok, bool has_pool);
bool ShouldInitializeMongo(bool enabled);
bool HasRequiredConfig(bool uri_empty, bool database_empty);
bool ShouldUseDefaultMomentsCollection(bool collection_empty);
bool CanEnsureIndexes(bool has_pool);
bool HasPositiveMomentId(long long moment_id);
bool CanAccessMomentContent(bool enabled, long long moment_id);
} // namespace memochat::tests::gate::mongo_dao

TEST(MongoDaoAlgorithmsTest, ParsesOnlyTruthyMongoConfigText)
{
    using namespace memochat::tests::gate::mongo_dao;

    EXPECT_TRUE(ParseBoolText("1"));
    EXPECT_TRUE(ParseBoolText("TRUE"));
    EXPECT_TRUE(ParseBoolText("Yes"));
    EXPECT_TRUE(ParseBoolText("on"));

    EXPECT_FALSE(ParseBoolText(nullptr));
    EXPECT_FALSE(ParseBoolText(""));
    EXPECT_FALSE(ParseBoolText(" true"));
    EXPECT_FALSE(ParseBoolText("false"));
    EXPECT_FALSE(ParseBoolText("enabled"));
}

TEST(MongoDaoAlgorithmsTest, ExposesDefaultCollectionAndInitializationGuards)
{
    using namespace memochat::tests::gate::mongo_dao;

    EXPECT_STREQ(DefaultMomentsCollection(), "moments_content");
    EXPECT_TRUE(ShouldInitializeMongo(true));
    EXPECT_FALSE(ShouldInitializeMongo(false));

    EXPECT_TRUE(IsEnabled(true, true, true));
    EXPECT_FALSE(IsEnabled(false, true, true));
    EXPECT_FALSE(IsEnabled(true, false, true));
    EXPECT_FALSE(IsEnabled(true, true, false));

    EXPECT_TRUE(HasRequiredConfig(false, false));
    EXPECT_FALSE(HasRequiredConfig(true, false));
    EXPECT_FALSE(HasRequiredConfig(false, true));
    EXPECT_TRUE(ShouldUseDefaultMomentsCollection(true));
    EXPECT_FALSE(ShouldUseDefaultMomentsCollection(false));
    EXPECT_TRUE(CanEnsureIndexes(true));
    EXPECT_FALSE(CanEnsureIndexes(false));
}

TEST(MongoDaoAlgorithmsTest, GatesMomentContentAccess)
{
    using namespace memochat::tests::gate::mongo_dao;

    EXPECT_FALSE(HasPositiveMomentId(0));
    EXPECT_FALSE(HasPositiveMomentId(-1));
    EXPECT_TRUE(HasPositiveMomentId(1));

    EXPECT_TRUE(CanAccessMomentContent(true, 42));
    EXPECT_FALSE(CanAccessMomentContent(false, 42));
    EXPECT_FALSE(CanAccessMomentContent(true, 0));
    EXPECT_FALSE(CanAccessMomentContent(true, -42));
}
