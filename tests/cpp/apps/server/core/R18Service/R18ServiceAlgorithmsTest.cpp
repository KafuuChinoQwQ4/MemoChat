#include <gtest/gtest.h>

namespace memochat::tests::r18::service
{
const char* DefaultSourceId();
const char* EmptyFieldDefault();
const char* TokenInvalidMessage();
const char* ImportFileNameField();
const char* ImportDataBase64Field();
const char* ImportManifestJsonField();
const char* DefaultImportFileName();
bool ShouldRejectImportPayload(bool encoded_empty, bool decode_ok);
const char* InvalidPluginPackagePayloadMessage();
const char* DefaultSourceAdminHeader();
const char* SourceAdminRequiredMessage();
bool ShouldRejectSourceAdminAuth(bool key_configured, bool supplied_empty, bool token_matches);
int SuccessHttpStatus();
int UnauthorizedHttpStatus();
int ForbiddenHttpStatus();
int BadGatewayHttpStatus();
const char* GetJsonContentType();
const char* PostJsonContentType();
const char* PlainTextContentType();
const char* ImageFetchFailedPrefix();
} // namespace memochat::tests::r18::service

TEST(R18ServiceAlgorithmsTest, ExposesStableAuthAndImportPayloadDefaults)
{
    using namespace memochat::tests::r18::service;

    EXPECT_STREQ(DefaultSourceId(), "");
    EXPECT_STREQ(EmptyFieldDefault(), "");
    EXPECT_STREQ(TokenInvalidMessage(), "token invalid");
    EXPECT_STREQ(ImportFileNameField(), "file_name");
    EXPECT_STREQ(ImportDataBase64Field(), "data_base64");
    EXPECT_STREQ(ImportManifestJsonField(), "manifest_json");
    EXPECT_STREQ(DefaultImportFileName(), "source.js");
    EXPECT_TRUE(ShouldRejectImportPayload(true, false));
    EXPECT_TRUE(ShouldRejectImportPayload(true, true));
    EXPECT_TRUE(ShouldRejectImportPayload(false, false));
    EXPECT_FALSE(ShouldRejectImportPayload(false, true));
    EXPECT_STREQ(InvalidPluginPackagePayloadMessage(), "invalid plugin package payload");
}

TEST(R18ServiceAlgorithmsTest, SourceAdminAuthorizationFailsClosed)
{
    using namespace memochat::tests::r18::service;

    EXPECT_STREQ(DefaultSourceAdminHeader(), "X-MemoChat-R18-Source-Admin-Key");
    EXPECT_STREQ(SourceAdminRequiredMessage(), "R18 source administrator authorization required");
    EXPECT_TRUE(ShouldRejectSourceAdminAuth(false, true, false));
    EXPECT_TRUE(ShouldRejectSourceAdminAuth(false, false, true));
    EXPECT_TRUE(ShouldRejectSourceAdminAuth(true, true, false));
    EXPECT_TRUE(ShouldRejectSourceAdminAuth(true, false, false));
    EXPECT_FALSE(ShouldRejectSourceAdminAuth(true, false, true));
}

TEST(R18ServiceAlgorithmsTest, ExposesStableResponseMetadata)
{
    using namespace memochat::tests::r18::service;

    EXPECT_EQ(SuccessHttpStatus(), 200);
    EXPECT_EQ(UnauthorizedHttpStatus(), 401);
    EXPECT_EQ(ForbiddenHttpStatus(), 403);
    EXPECT_EQ(BadGatewayHttpStatus(), 502);
    EXPECT_STREQ(GetJsonContentType(), "text/json");
    EXPECT_STREQ(PostJsonContentType(), "application/json");
    EXPECT_STREQ(PlainTextContentType(), "text/plain");
    EXPECT_STREQ(ImageFetchFailedPrefix(), "image fetch failed: ");
}
