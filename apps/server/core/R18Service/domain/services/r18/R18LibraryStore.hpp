#pragma once

#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace memochat::r18
{

struct R18LibraryFolder
{
    std::string id;
    std::string name;
    int64_t created_at_ms = 0;
    int64_t updated_at_ms = 0;
};

struct R18LibraryItem
{
    std::string source_id;
    std::string comic_id;
    std::string title;
    std::string cover;
    std::string author;
    std::string subtitle;
    std::vector<std::string> folder_ids;
    int64_t favorited_at_ms = 0;
    int64_t updated_at_ms = 0;
};

// Per-MemoChat-user library: custom folders + favorited comics.
// Stored under data/r18/library/<uid>.json.
class R18LibraryStore
{
public:
    static R18LibraryStore& Instance();

    static constexpr const char* kDefaultFolderId = "default";
    static constexpr const char* kDefaultFolderName = "默认收藏";

    memochat::json::JsonValue ListLibrary(int uid);
    memochat::json::JsonValue ListFolders(int uid);
    memochat::json::JsonValue ListFavorites(int uid, const std::string& folder_id = {});

    bool IsFavorited(int uid, const std::string& source_id, const std::string& comic_id);

    // favorited=true upserts metadata; favorited=false removes the item.
    // folder_ids empty on favorite → put into default folder.
    bool ToggleFavorite(int uid,
                        const std::string& source_id,
                        const std::string& comic_id,
                        bool favorited,
                        const std::string& title,
                        const std::string& cover,
                        const std::string& author,
                        const std::string& subtitle,
                        const std::vector<std::string>& folder_ids,
                        memochat::json::JsonValue* out,
                        std::string* error);

    bool AssignFolders(int uid,
                       const std::string& source_id,
                       const std::string& comic_id,
                       const std::vector<std::string>& folder_ids,
                       memochat::json::JsonValue* out,
                       std::string* error);

    bool CreateFolder(int uid, const std::string& name, memochat::json::JsonValue* out, std::string* error);
    bool RenameFolder(int uid,
                      const std::string& folder_id,
                      const std::string& name,
                      memochat::json::JsonValue* out,
                      std::string* error);
    bool DeleteFolder(int uid, const std::string& folder_id, memochat::json::JsonValue* out, std::string* error);

private:
    struct UserLibrary
    {
        std::vector<R18LibraryFolder> folders;
        std::vector<R18LibraryItem> items;
    };

    R18LibraryStore();

    std::filesystem::path ResolveRoot() const;
    std::filesystem::path PathForUid(int uid) const;
    void EnsureRootLocked();
    void LoadUidLocked(int uid);
    void SaveUidLocked(int uid);
    void EnsureDefaultFolderLocked(UserLibrary& lib);
    std::string MakeFolderIdLocked();
    memochat::json::JsonValue FolderToJson(const R18LibraryFolder& folder) const;
    memochat::json::JsonValue ItemToJson(const R18LibraryItem& item) const;
    memochat::json::JsonValue LibraryToJson(const UserLibrary& lib) const;
    std::vector<std::string> SanitizeFolderIdsLocked(const UserLibrary& lib,
                                                     const std::vector<std::string>& folder_ids) const;

    mutable std::mutex mu_;
    std::filesystem::path root_;
    std::unordered_map<int, UserLibrary> by_uid_;
    int64_t folder_seq_ = 0;
};

} // namespace memochat::r18
