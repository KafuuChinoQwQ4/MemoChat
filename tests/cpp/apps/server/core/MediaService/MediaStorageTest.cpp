#include "MediaStorage.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

TEST(MediaStorageTest, LocalReadObjectLowercasesExtensionThroughImportedModule)
{
    std::error_code ec;
    const auto temp_directory = std::filesystem::temp_directory_path(ec);
    ASSERT_FALSE(ec);
    const auto root = temp_directory / "memochat_media_storage_module_test";
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    ASSERT_FALSE(ec);

    const auto file_path = root / "PHOTO.PNG";
    {
        std::ofstream out(file_path, std::ios::binary);
        out << "png-data";
    }

    LocalMediaStorage storage(root);
    ASSERT_TRUE(storage.Ready()) << storage.StartupError();
    std::vector<char> data;
    std::string content_type;
    std::string error;

    ASSERT_TRUE(storage.ReadObject("PHOTO.PNG", "image", data, content_type, error)) << error;
    EXPECT_EQ(std::string(data.begin(), data.end()), "png-data");
    EXPECT_EQ(content_type, "image/png");

    std::filesystem::remove_all(root, ec);
}

TEST(MediaStorageTest, LocalResolveReadPathRejectsAbsoluteAndParentTraversal)
{
    std::error_code ec;
    const auto temp_directory = std::filesystem::temp_directory_path(ec);
    ASSERT_FALSE(ec);
    const auto root = temp_directory / "memochat_media_storage_path_guard_test";
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "assets", ec);
    ASSERT_FALSE(ec);

    LocalMediaStorage storage(root);
    ASSERT_TRUE(storage.Ready()) << storage.StartupError();
    std::filesystem::path resolved;

    EXPECT_FALSE(storage.ResolveReadPath((root / "assets" / "photo.png").string(), resolved));
    EXPECT_FALSE(storage.ResolveReadPath("../outside.png", resolved));
    EXPECT_FALSE(storage.ResolveReadPath("assets/../outside.png", resolved));

    const auto inside = root / "assets" / "photo.png";
    {
        std::ofstream out(inside, std::ios::binary);
        out << "image";
    }

    ASSERT_TRUE(storage.ResolveReadPath("uploads/assets/photo.png", resolved));
    const auto expected = std::filesystem::weakly_canonical(inside, ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(expected, resolved);

    std::filesystem::remove_all(root, ec);
}

TEST(MediaStorageTest, LocalResolveReadPathRejectsCanonicalEscape)
{
    std::error_code ec;
    const auto temp_directory = std::filesystem::temp_directory_path(ec);
    ASSERT_FALSE(ec);
    const auto root = temp_directory / "memochat_media_storage_symlink_guard_test";
    const auto outside = temp_directory / "memochat_media_storage_symlink_outside.txt";
    std::filesystem::remove_all(root, ec);
    std::filesystem::remove(outside, ec);
    std::filesystem::create_directories(root, ec);
    ASSERT_FALSE(ec);
    {
        std::ofstream out(outside, std::ios::binary);
        out << "outside";
    }

    const auto link = root / "escape.txt";
    std::filesystem::create_symlink(outside, link, ec);
    if (ec)
    {
        GTEST_SKIP() << "symlink creation unavailable: " << ec.message();
    }

    LocalMediaStorage storage(root);
    ASSERT_TRUE(storage.Ready()) << storage.StartupError();
    std::filesystem::path resolved;
    EXPECT_FALSE(storage.ResolveReadPath("escape.txt", resolved));

    std::filesystem::remove_all(root, ec);
    std::filesystem::remove(outside, ec);
}

TEST(MediaStorageTest, LocalPublicUrlRequiresExplicitRedirectConfig)
{
    std::error_code ec;
    const auto temp_directory = std::filesystem::temp_directory_path(ec);
    ASSERT_FALSE(ec);
    LocalMediaStorage storage(temp_directory / "memochat_media_storage_redirect_guard_test");
    ASSERT_TRUE(storage.Ready()) << storage.StartupError();
    std::string public_url;

    EXPECT_FALSE(storage.ResolvePublicUrl("https://example.test/object", "image", public_url));
    EXPECT_TRUE(public_url.empty());
}

TEST(MediaStorageTest, LocalStorageReportsUnwritableRootWithoutThrowing)
{
    std::error_code ec;
    const auto temp_directory = std::filesystem::temp_directory_path(ec);
    ASSERT_FALSE(ec);
    const auto blocker = temp_directory / "memochat_media_storage_root_blocker";
    std::filesystem::remove_all(blocker, ec);
    {
        std::ofstream out(blocker, std::ios::binary);
        out << "not-a-directory";
    }

    LocalMediaStorage storage(blocker);
    EXPECT_FALSE(storage.Ready());
    EXPECT_FALSE(storage.StartupError().empty());

    std::string storage_path;
    std::string error;
    EXPECT_FALSE(storage.StoreMergedFile("image", "key", "photo.png", blocker, storage_path, error));
    EXPECT_EQ(error, storage.StartupError());

    std::filesystem::remove(blocker, ec);
}
