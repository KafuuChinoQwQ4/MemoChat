#include <gtest/gtest.h>

#include "services/media/MediaUploadSessionDto.hpp"
#include "json/GlazeCompat.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadSessionDto>(
    std::array<std::string_view, 15>{"uid",
                                     "upload_id",
                                     "media_type",
                                     "file_name",
                                     "mime",
                                     "file_size",
                                     "chunk_size",
                                     "total_chunks",
                                     "created_at",
                                     "expires_at",
                                     "storage_provider",
                                     "grant_uids",
                                     "grant_group_id",
                                     "grant_public",
                                     "grant_friends"}));
#endif

namespace
{

memochat::media::MediaUploadSessionDto MakeSession()
{
    memochat::media::MediaUploadSessionDto session;
    session.uid = 42;
    session.upload_id = "upload-1";
    session.media_type = "image";
    session.file_name = "photo.png";
    session.mime = "image/png";
    session.file_size = 4096;
    session.chunk_size = 1024;
    session.total_chunks = 4;
    session.created_at = 1000;
    session.expires_at = 2000;
    session.storage_provider = "local";
    session.grant_uids = {7, 8};
    session.grant_group_id = 99;
    session.grant_public = true;
    session.grant_friends = false;
    return session;
}

} // namespace

TEST(MediaUploadSessionDtoTest, EncodesSessionWithExistingWireFieldNames)
{
    std::string body;
    ASSERT_TRUE(memochat::media::EncodeMediaUploadSession(MakeSession(), &body));

    EXPECT_NE(body.find(R"("uid":42)"), std::string::npos);
    EXPECT_NE(body.find(R"("upload_id":"upload-1")"), std::string::npos);
    EXPECT_NE(body.find(R"("media_type":"image")"), std::string::npos);
    EXPECT_NE(body.find(R"("file_name":"photo.png")"), std::string::npos);
    EXPECT_NE(body.find(R"("mime":"image/png")"), std::string::npos);
    EXPECT_NE(body.find(R"("file_size":4096)"), std::string::npos);
    EXPECT_NE(body.find(R"("chunk_size":1024)"), std::string::npos);
    EXPECT_NE(body.find(R"("total_chunks":4)"), std::string::npos);
    EXPECT_NE(body.find(R"("created_at":1000)"), std::string::npos);
    EXPECT_NE(body.find(R"("expires_at":2000)"), std::string::npos);
    EXPECT_NE(body.find(R"("storage_provider":"local")"), std::string::npos);
    EXPECT_NE(body.find(R"("grant_uids":[7,8])"), std::string::npos);
    EXPECT_NE(body.find(R"("grant_group_id":99)"), std::string::npos);
    EXPECT_NE(body.find(R"("grant_public":true)"), std::string::npos);
    EXPECT_NE(body.find(R"("grant_friends":false)"), std::string::npos);
}

TEST(MediaUploadSessionDtoTest, DecodesFullSession)
{
    memochat::media::MediaUploadSessionDto session;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-2","media_type":"file","file_name":"doc.pdf","mime":"application/pdf","file_size":2048,"chunk_size":1024,"total_chunks":2,"created_at":10,"expires_at":20,"storage_provider":"s3","grant_uids":[9,9,0,10],"grant_group_id":123,"grant_public":false,"grant_friends":true})",
        &session));

    EXPECT_EQ(session.uid, 7);
    EXPECT_EQ(session.upload_id, "upload-2");
    EXPECT_EQ(session.media_type, "file");
    EXPECT_EQ(session.file_name, "doc.pdf");
    EXPECT_EQ(session.mime, "application/pdf");
    EXPECT_EQ(session.file_size, 2048);
    EXPECT_EQ(session.chunk_size, 1024);
    EXPECT_EQ(session.total_chunks, 2);
    EXPECT_EQ(session.created_at, 10);
    EXPECT_EQ(session.expires_at, 20);
    EXPECT_EQ(session.storage_provider, "s3");
    ASSERT_EQ(session.grant_uids.size(), 2U);
    EXPECT_EQ(session.grant_uids[0], 9);
    EXPECT_EQ(session.grant_uids[1], 10);
    EXPECT_EQ(session.grant_group_id, 123);
    EXPECT_FALSE(session.grant_public);
    EXPECT_TRUE(session.grant_friends);
}

TEST(MediaUploadSessionDtoTest, DecodesMissingOptionalFieldsWithDefaults)
{
    memochat::media::MediaUploadSessionDto session;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-3","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));

    EXPECT_EQ(session.uid, 7);
    EXPECT_EQ(session.upload_id, "upload-3");
    EXPECT_EQ(session.media_type, "file");
    EXPECT_EQ(session.file_name, "file.bin");
    EXPECT_TRUE(session.mime.empty());
    EXPECT_EQ(session.file_size, 1);
    EXPECT_EQ(session.chunk_size, 1);
    EXPECT_EQ(session.total_chunks, 1);
    EXPECT_EQ(session.created_at, 0);
    EXPECT_EQ(session.expires_at, 0);
    EXPECT_EQ(session.storage_provider, "local");
    EXPECT_TRUE(session.grant_uids.empty());
    EXPECT_EQ(session.grant_group_id, 0);
    EXPECT_FALSE(session.grant_public);
    EXPECT_FALSE(session.grant_friends);
}

TEST(MediaUploadSessionDtoTest, RejectsInvalidSession)
{
    memochat::media::MediaUploadSessionDto session;

    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession("not-json", &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":0,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":0,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":0,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":0})",
        &session));
    EXPECT_FALSE(
        memochat::media::DecodeMediaUploadSession(R"({"uid":7})",
                                                  static_cast<memochat::media::MediaUploadSessionDto*>(nullptr)));
}

TEST(MediaUploadSessionDtoTest, BridgesSessionToAndFromJsonValue)
{
    const memochat::json::JsonValue value = memochat::media::MediaUploadSessionToJsonValue(MakeSession());
    EXPECT_EQ(value["upload_id"].asString(), "upload-1");
    EXPECT_EQ(value["file_name"].asString(), "photo.png");

    memochat::media::MediaUploadSessionDto decoded;
    ASSERT_TRUE(memochat::media::MediaUploadSessionFromJsonValue(value, &decoded));
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_EQ(decoded.upload_id, "upload-1");
    EXPECT_EQ(decoded.media_type, "image");
    EXPECT_EQ(decoded.storage_provider, "local");
    ASSERT_EQ(decoded.grant_uids.size(), 2U);
    EXPECT_EQ(decoded.grant_group_id, 99);
    EXPECT_TRUE(decoded.grant_public);
    EXPECT_FALSE(decoded.grant_friends);
}

TEST(MediaUploadSessionDtoTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(memochat::media::EncodeMediaUploadSession(MakeSession(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
