#include "r18/R18SourceService.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(R18SourceServiceTest, ImportZipNormalizesIdAndVersionThroughImportedModule)
{
    std::error_code ec;
    const auto original_cwd = std::filesystem::current_path(ec);
    ASSERT_FALSE(ec);
    const auto temp_root = std::filesystem::temp_directory_path(ec) / "memochat_r18_source_module_test";
    ASSERT_FALSE(ec);
    std::filesystem::remove_all(temp_root, ec);
    std::filesystem::create_directories(temp_root, ec);
    ASSERT_FALSE(ec);
    std::filesystem::current_path(temp_root, ec);
    ASSERT_FALSE(ec);

    std::string error;
    const std::string manifest =
        R"({"id":"  My Source++  ","name":"Imported","version":"../v 1.0//","format":"source-js"})";
    const std::string script = "class ComicSource { async search() { return []; } }";

    const auto record =
        memochat::r18::R18SourceService::Instance().ImportZip("My Source++.js", manifest, script, &error);

    ASSERT_TRUE(error.empty()) << error;
    EXPECT_EQ(record.id, "my-source");
    EXPECT_EQ(record.version, "v-1.0");
    EXPECT_EQ(record.format, "source-js");
    EXPECT_EQ(record.status, "staged-js");

    std::filesystem::current_path(original_cwd, ec);
    std::filesystem::remove_all(temp_root, ec);
}

TEST(R18SourceServiceTest, DisabledImportedSourceCannotDispatch)
{
    auto& service = memochat::r18::R18SourceService::Instance();
    std::string error;
    const std::string manifest = R"({"id":"disabled-source","name":"Disabled","version":"1.0.0","format":"source-js"})";
    const auto record = service.ImportZip("disabled-source.js",
                                          manifest,
                                          "class ComicSource { async search() { return []; } }",
                                          &error);
    ASSERT_TRUE(error.empty()) << error;
    ASSERT_FALSE(record.enabled);

    const auto denied = service.Search(record.id, "query", 1);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(denied, "error_message", ""), "source is disabled");

    ASSERT_TRUE(service.EnableSource(record.id, true, &error)) << error;
    const auto allowed = service.Search(record.id, "query", 1);
    EXPECT_TRUE(memochat::json::glaze_safe_get<std::string>(allowed, "error_message", "").empty());
}
