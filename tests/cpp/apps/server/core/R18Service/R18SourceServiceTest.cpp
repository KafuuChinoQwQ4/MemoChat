#include "r18/R18SourceService.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

TEST(R18SourceServiceTest, ImportZipNormalizesIdAndVersionThroughImportedModule)
{
    const auto original_cwd = std::filesystem::current_path();
    const auto temp_root = std::filesystem::temp_directory_path() / "memochat_r18_source_module_test";
    std::error_code ec;
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
