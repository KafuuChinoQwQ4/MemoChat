#include <gtest/gtest.h>

#include "r18/R18SourceRecordCodec.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18SourceRecordDto>(
    std::array<std::string_view, 11>{"id",
                                     "name",
                                     "version",
                                     "path",
                                     "format",
                                     "source_url",
                                     "catalog_url",
                                     "enabled",
                                     "builtin",
                                     "status",
                                     "message"}));
#endif

namespace
{

memochat::r18::R18SourceRecord MakeRecord()
{
    memochat::r18::R18SourceRecord record;
    record.id = "source-1";
    record.name = "Source One";
    record.version = "1.2.3";
    record.path = "/data/source.zip";
    record.format = "source-js";
    record.source_url = "https://example.com/source";
    record.catalog_url = "https://example.com/catalog";
    record.enabled = true;
    record.builtin = true;
    record.status = "ok";
    record.message = "ready";
    return record;
}

} // namespace

TEST(R18SourceRecordCodecTest, EncodesSourceRecordWithExistingWireFieldNames)
{
    std::string body;
    ASSERT_TRUE(memochat::r18::EncodeR18SourceRecord(MakeRecord(), &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["id"].asString(), "source-1");
    EXPECT_EQ(root["name"].asString(), "Source One");
    EXPECT_EQ(root["version"].asString(), "1.2.3");
    EXPECT_EQ(root["path"].asString(), "/data/source.zip");
    EXPECT_EQ(root["format"].asString(), "source-js");
    EXPECT_EQ(root["source_url"].asString(), "https://example.com/source");
    EXPECT_EQ(root["catalog_url"].asString(), "https://example.com/catalog");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(root, "enabled", false));
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(root, "builtin", false));
    EXPECT_EQ(root["status"].asString(), "ok");
    EXPECT_EQ(root["message"].asString(), "ready");
}

TEST(R18SourceRecordCodecTest, DecodesFullSourceRecord)
{
    memochat::r18::R18SourceRecord record;
    ASSERT_TRUE(memochat::r18::DecodeR18SourceRecord(
        R"({"id":"source-2","name":"Two","version":"2.0.0","path":"/tmp/two.zip","format":"native-zip","source_url":"s","catalog_url":"c","enabled":true,"builtin":false,"status":"staged","message":"msg"})",
        &record));

    EXPECT_EQ(record.id, "source-2");
    EXPECT_EQ(record.name, "Two");
    EXPECT_EQ(record.version, "2.0.0");
    EXPECT_EQ(record.path, "/tmp/two.zip");
    EXPECT_EQ(record.format, "native-zip");
    EXPECT_EQ(record.source_url, "s");
    EXPECT_EQ(record.catalog_url, "c");
    EXPECT_TRUE(record.enabled);
    EXPECT_FALSE(record.builtin);
    EXPECT_EQ(record.status, "staged");
    EXPECT_EQ(record.message, "msg");
}

TEST(R18SourceRecordCodecTest, DecodesMissingOptionalFieldsWithLegacyManifestDefaults)
{
    memochat::r18::R18SourceRecord record;
    ASSERT_TRUE(memochat::r18::DecodeR18SourceRecord(R"({"id":"source-3"})", &record));

    EXPECT_EQ(record.id, "source-3");
    EXPECT_EQ(record.name, "source-3");
    EXPECT_EQ(record.version, "0.0.0");
    EXPECT_TRUE(record.path.empty());
    EXPECT_EQ(record.format, "native-zip");
    EXPECT_TRUE(record.source_url.empty());
    EXPECT_TRUE(record.catalog_url.empty());
    EXPECT_FALSE(record.enabled);
    EXPECT_FALSE(record.builtin);
    EXPECT_EQ(record.status, "staged");
    EXPECT_TRUE(record.message.empty());
}

TEST(R18SourceRecordCodecTest, RejectsInvalidSourceRecord)
{
    memochat::r18::R18SourceRecord record;

    EXPECT_FALSE(memochat::r18::DecodeR18SourceRecord("not-json", &record));
    EXPECT_FALSE(memochat::r18::DecodeR18SourceRecord(R"({"id":""})", &record));
    EXPECT_FALSE(memochat::r18::DecodeR18SourceRecord(R"({"name":"missing id"})", &record));
    EXPECT_FALSE(memochat::r18::DecodeR18SourceRecord(R"({"id":"source-4"})",
                                                      static_cast<memochat::r18::R18SourceRecord*>(nullptr)));
}

TEST(R18SourceRecordCodecTest, BridgesRecordToAndFromJsonValue)
{
    const memochat::json::JsonValue value = memochat::r18::R18SourceRecordToJsonValue(MakeRecord());
    EXPECT_EQ(value["id"].asString(), "source-1");
    EXPECT_EQ(value["format"].asString(), "source-js");

    memochat::r18::R18SourceRecord decoded;
    ASSERT_TRUE(memochat::r18::R18SourceRecordFromJsonValue(value, &decoded));
    EXPECT_EQ(decoded.id, "source-1");
    EXPECT_EQ(decoded.name, "Source One");
    EXPECT_EQ(decoded.version, "1.2.3");
    EXPECT_TRUE(decoded.enabled);
    EXPECT_TRUE(decoded.builtin);
}

TEST(R18SourceRecordCodecTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(memochat::r18::EncodeR18SourceRecord(MakeRecord(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
