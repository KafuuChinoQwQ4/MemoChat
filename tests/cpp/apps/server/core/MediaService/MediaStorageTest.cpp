#include "MediaStorage.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

TEST(MediaStorageTest, LocalReadObjectLowercasesExtensionThroughImportedModule)
{
    const auto root = std::filesystem::temp_directory_path() / "memochat_media_storage_module_test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    ASSERT_FALSE(ec);

    const auto file_path = root / "PHOTO.PNG";
    {
        std::ofstream out(file_path, std::ios::binary);
        out << "png-data";
    }

    LocalMediaStorage storage(root);
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
    const auto root = std::filesystem::temp_directory_path() / "memochat_media_storage_path_guard_test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "assets", ec);
    ASSERT_FALSE(ec);

    LocalMediaStorage storage(root);
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
    const auto root = std::filesystem::temp_directory_path() / "memochat_media_storage_symlink_guard_test";
    const auto outside = std::filesystem::temp_directory_path() / "memochat_media_storage_symlink_outside.txt";
    std::error_code ec;
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
    std::filesystem::path resolved;
    EXPECT_FALSE(storage.ResolveReadPath("escape.txt", resolved));

    std::filesystem::remove_all(root, ec);
    std::filesystem::remove(outside, ec);
}

TEST(MediaStorageTest, LocalPublicUrlRequiresExplicitRedirectConfig)
{
    LocalMediaStorage storage(std::filesystem::temp_directory_path() / "memochat_media_storage_redirect_guard_test");
    std::string public_url;

    EXPECT_FALSE(storage.ResolvePublicUrl("https://example.test/object", "image", public_url));
    EXPECT_TRUE(public_url.empty());
}
