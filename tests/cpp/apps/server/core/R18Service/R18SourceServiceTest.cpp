#include "r18/R18SourceService.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <cstdlib>
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
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(denied, "error_message", ""),
              "JavaScript source saved. Execution requires a MemoChat source runtime adapter.");

    ASSERT_TRUE(service.EnableSource(record.id, true, &error)) << error;
    const auto allowed = service.Search(record.id, "query", 1);
    EXPECT_TRUE(memochat::json::glaze_safe_get<std::string>(allowed, "error_message", "").empty());
}

TEST(R18SourceServiceTest, PicacgWithoutAccountIsAuthRequiredInListSources)
{
    auto& service = memochat::r18::R18SourceService::Instance();
    const auto sources = service.ListSourcesForUser(4242);
    ASSERT_TRUE(sources.is_array());

    bool found_picacg = false;
    bool found_nhentai = false;
    bool found_ehentai = false;
    for (std::size_t i = 0; i < sources.size(); ++i)
    {
        const auto source = sources[static_cast<int>(i)];
        const auto id = memochat::json::glaze_safe_get<std::string>(source, "id", "");
        if (id == "picacg.official")
        {
            found_picacg = true;
            EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(source, "enabled", true));
            EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(source, "status", ""), "auth-required");
            EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(source, "message", ""),
                      "Picacg account login required");
        }
        if (id == "nhentai.official")
        {
            found_nhentai = true;
            EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(source, "enabled", false));
        }
        if (id == "ehentai.official")
        {
            found_ehentai = true;
            EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(source, "enabled", false));
        }
    }
    EXPECT_TRUE(found_picacg);
    EXPECT_TRUE(found_nhentai);
    EXPECT_TRUE(found_ehentai);

    const auto denied = service.SearchForUser(4242, "picacg.official", "query", 1);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(denied, "error_message", ""),
              "Picacg account login required");
    const auto items = denied["items"];
    ASSERT_TRUE(items.is_array());
    ASSERT_GE(items.size(), 1U);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(items[0], "title", ""), "官方源请求失败");
}

TEST(R18SourceServiceTest, AccountManagerSavesAndListsWithoutExposingSecrets)
{
    auto& service = memochat::r18::R18SourceService::Instance();
    std::string error;
    ASSERT_TRUE(
        service.SaveAccount(777, "ehentai.official", "cookie-user", "ipb_member_id=1; ipb_pass_hash=abc", &error))
        << error;
    const auto accounts = service.ListAccounts(777);
    const auto managed = accounts["managed"];
    ASSERT_TRUE(managed.is_array());
    bool found = false;
    for (std::size_t i = 0; i < managed.size(); ++i)
    {
        const auto item = managed[static_cast<int>(i)];
        if (memochat::json::glaze_safe_get<std::string>(item, "source_id", "") != "ehentai.official")
            continue;
        found = true;
        EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(item, "username", ""), "cookie-user");
        EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(item, "has_password", false));
        EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(item, "has_session", false) ||
                    memochat::json::glaze_safe_get<bool>(item, "has_session", true));
        EXPECT_FALSE(memochat::json::glaze_has_key(item, "password"));
        EXPECT_FALSE(memochat::json::glaze_has_key(item, "session_cookie"));
    }
    EXPECT_TRUE(found);
    ASSERT_TRUE(service.ClearAccount(777, "ehentai.official", &error)) << error;
}
