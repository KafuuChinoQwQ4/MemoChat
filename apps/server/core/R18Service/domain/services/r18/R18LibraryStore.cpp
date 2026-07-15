#include "r18/R18LibraryStore.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

namespace memochat::r18
{
namespace
{

using json::JsonValue;

int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string Trim(std::string value)
{
    while (!value.empty() &&
           (value.front() == ' ' || value.front() == '\t' || value.front() == '\n' || value.front() == '\r'))
        value.erase(value.begin());
    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\t' || value.back() == '\n' || value.back() == '\r'))
        value.pop_back();
    return value;
}

} // namespace

R18LibraryStore& R18LibraryStore::Instance()
{
    static R18LibraryStore store;
    return store;
}

R18LibraryStore::R18LibraryStore()
{
    root_ = ResolveRoot();
    EnsureRootLocked();
}

std::filesystem::path R18LibraryStore::ResolveRoot() const
{
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec)
    {
        const auto local = cwd / "data" / "r18" / "library";
        if (std::filesystem::exists(local, ec) || std::filesystem::create_directories(local, ec) || !ec)
            return local;
    }
    return std::filesystem::temp_directory_path() / "memochat_r18_library";
}

std::filesystem::path R18LibraryStore::PathForUid(int uid) const
{
    return root_ / (std::to_string(uid) + ".json");
}

void R18LibraryStore::EnsureRootLocked()
{
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
}

void R18LibraryStore::EnsureDefaultFolderLocked(UserLibrary& lib)
{
    const auto it = std::find_if(lib.folders.begin(),
                                 lib.folders.end(),
                                 [](const R18LibraryFolder& f)
                                 {
                                     return f.id == kDefaultFolderId;
                                 });
    if (it != lib.folders.end())
        return;
    R18LibraryFolder folder;
    folder.id = kDefaultFolderId;
    folder.name = kDefaultFolderName;
    folder.created_at_ms = NowMs();
    folder.updated_at_ms = folder.created_at_ms;
    lib.folders.insert(lib.folders.begin(), std::move(folder));
}

std::string R18LibraryStore::MakeFolderIdLocked()
{
    ++folder_seq_;
    std::ostringstream oss;
    oss << "fld_" << NowMs() << "_" << folder_seq_;
    return oss.str();
}

void R18LibraryStore::LoadUidLocked(int uid)
{
    if (by_uid_.find(uid) != by_uid_.end())
        return;

    UserLibrary lib;
    const auto path = PathForUid(uid);
    std::error_code ec;
    if (std::filesystem::exists(path, ec))
    {
        std::ifstream in(path, std::ios::binary);
        if (in.is_open())
        {
            const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            JsonValue root;
            if (json::glaze_parse(root, content) && root.is_object())
            {
                if (const auto* folders = json::glaze_get_array(json::glaze_get(root, "folders")))
                {
                    for (const auto& entry : *folders)
                    {
                        JsonValue item(entry);
                        R18LibraryFolder folder;
                        folder.id = json::glaze_safe_get<std::string>(item, "id", "");
                        folder.name = json::glaze_safe_get<std::string>(item, "name", "");
                        folder.created_at_ms = json::glaze_safe_get<int64_t>(item, "created_at_ms", 0);
                        folder.updated_at_ms =
                            json::glaze_safe_get<int64_t>(item, "updated_at_ms", folder.created_at_ms);
                        if (!folder.id.empty() && !folder.name.empty())
                            lib.folders.push_back(std::move(folder));
                    }
                }
                if (const auto* items = json::glaze_get_array(json::glaze_get(root, "items")))
                {
                    for (const auto& entry : *items)
                    {
                        JsonValue item(entry);
                        R18LibraryItem comic;
                        comic.source_id = json::glaze_safe_get<std::string>(item, "source_id", "");
                        comic.comic_id = json::glaze_safe_get<std::string>(item, "comic_id", "");
                        comic.title = json::glaze_safe_get<std::string>(item, "title", "");
                        comic.cover = json::glaze_safe_get<std::string>(item, "cover", "");
                        comic.author = json::glaze_safe_get<std::string>(item, "author", "");
                        comic.subtitle = json::glaze_safe_get<std::string>(item, "subtitle", "");
                        comic.favorited_at_ms = json::glaze_safe_get<int64_t>(item, "favorited_at_ms", 0);
                        comic.updated_at_ms =
                            json::glaze_safe_get<int64_t>(item, "updated_at_ms", comic.favorited_at_ms);
                        comic.folder_ids.clear();
                        const JsonValue folder_ids_value = json::glaze_get(item, "folder_ids");
                        if (const auto* arr = json::glaze_get_array(folder_ids_value))
                        {
                            for (const auto& fid : *arr)
                            {
                                JsonValue v(fid);
                                if (!v.isString())
                                    continue;
                                const std::string id = v.asString();
                                if (!id.empty())
                                    comic.folder_ids.push_back(id);
                            }
                        }
                        if (!comic.source_id.empty() && !comic.comic_id.empty())
                            lib.items.push_back(std::move(comic));
                    }
                }
            }
        }
    }

    EnsureDefaultFolderLocked(lib);
    // Drop unknown folder refs; ensure every item has at least default when empty.
    std::unordered_map<std::string, bool> valid;
    for (const auto& f : lib.folders)
        valid[f.id] = true;
    for (auto& item : lib.items)
    {
        std::vector<std::string> kept;
        for (const auto& fid : item.folder_ids)
        {
            if (valid.count(fid))
                kept.push_back(fid);
        }
        if (kept.empty())
            kept.push_back(kDefaultFolderId);
        item.folder_ids = std::move(kept);
    }
    by_uid_[uid] = std::move(lib);
}

void R18LibraryStore::SaveUidLocked(int uid)
{
    EnsureRootLocked();
    auto it = by_uid_.find(uid);
    if (it == by_uid_.end())
        return;
    EnsureDefaultFolderLocked(it->second);
    const JsonValue root = LibraryToJson(it->second);
    std::ofstream out(PathForUid(uid), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return;
    out << json::glaze_stringify(root);
}

JsonValue R18LibraryStore::FolderToJson(const R18LibraryFolder& folder) const
{
    JsonValue item;
    item["id"] = folder.id;
    item["name"] = folder.name;
    item["created_at_ms"] = folder.created_at_ms;
    item["updated_at_ms"] = folder.updated_at_ms;
    return item;
}

JsonValue R18LibraryStore::ItemToJson(const R18LibraryItem& item) const
{
    JsonValue out;
    out["source_id"] = item.source_id;
    out["comic_id"] = item.comic_id;
    out["title"] = item.title;
    out["cover"] = item.cover;
    out["author"] = item.author;
    out["subtitle"] = item.subtitle;
    out["favorited_at_ms"] = item.favorited_at_ms;
    out["updated_at_ms"] = item.updated_at_ms;
    out["favorited"] = true;
    JsonValue folders{json::array_t{}};
    for (const auto& fid : item.folder_ids)
        json::glaze_append(folders, fid);
    out["folder_ids"] = folders;
    return out;
}

JsonValue R18LibraryStore::LibraryToJson(const UserLibrary& lib) const
{
    JsonValue root;
    JsonValue folders{json::array_t{}};
    for (const auto& folder : lib.folders)
        json::glaze_append(folders, FolderToJson(folder));
    JsonValue items{json::array_t{}};
    for (const auto& item : lib.items)
        json::glaze_append(items, ItemToJson(item));
    root["folders"] = folders;
    root["items"] = items;
    return root;
}

std::vector<std::string> R18LibraryStore::SanitizeFolderIdsLocked(const UserLibrary& lib,
                                                                  const std::vector<std::string>& folder_ids) const
{
    std::unordered_map<std::string, bool> valid;
    for (const auto& f : lib.folders)
        valid[f.id] = true;
    std::vector<std::string> out;
    for (const auto& fid : folder_ids)
    {
        if (fid.empty() || !valid.count(fid))
            continue;
        if (std::find(out.begin(), out.end(), fid) == out.end())
            out.push_back(fid);
    }
    if (out.empty())
        out.push_back(kDefaultFolderId);
    return out;
}

JsonValue R18LibraryStore::ListLibrary(int uid)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    return LibraryToJson(by_uid_[uid]);
}

JsonValue R18LibraryStore::ListFolders(int uid)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    JsonValue folders{json::array_t{}};
    for (const auto& folder : by_uid_[uid].folders)
        json::glaze_append(folders, FolderToJson(folder));
    JsonValue data;
    data["folders"] = folders;
    return data;
}

JsonValue R18LibraryStore::ListFavorites(int uid, const std::string& folder_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    JsonValue items{json::array_t{}};
    for (const auto& item : by_uid_[uid].items)
    {
        if (!folder_id.empty())
        {
            if (std::find(item.folder_ids.begin(), item.folder_ids.end(), folder_id) == item.folder_ids.end())
                continue;
        }
        json::glaze_append(items, ItemToJson(item));
    }
    JsonValue data;
    data["folder_id"] = folder_id;
    data["items"] = items;
    return data;
}

bool R18LibraryStore::IsFavorited(int uid, const std::string& source_id, const std::string& comic_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    for (const auto& item : by_uid_[uid].items)
    {
        if (item.source_id == source_id && item.comic_id == comic_id)
            return true;
    }
    return false;
}

bool R18LibraryStore::ToggleFavorite(int uid,
                                     const std::string& source_id,
                                     const std::string& comic_id,
                                     bool favorited,
                                     const std::string& title,
                                     const std::string& cover,
                                     const std::string& author,
                                     const std::string& subtitle,
                                     const std::vector<std::string>& folder_ids,
                                     JsonValue* out,
                                     std::string* error)
{
    const std::string sid = Trim(source_id);
    const std::string cid = Trim(comic_id);
    if (sid.empty() || cid.empty())
    {
        if (error)
            *error = "source_id and comic_id are required";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& lib = by_uid_[uid];
    EnsureDefaultFolderLocked(lib);

    auto find_it = [&]()
    {
        return std::find_if(lib.items.begin(),
                            lib.items.end(),
                            [&](const R18LibraryItem& item)
                            {
                                return item.source_id == sid && item.comic_id == cid;
                            });
    };

    if (!favorited)
    {
        auto it = find_it();
        if (it != lib.items.end())
            lib.items.erase(it);
        SaveUidLocked(uid);
        if (out)
        {
            JsonValue data;
            data["source_id"] = sid;
            data["comic_id"] = cid;
            data["favorited"] = false;
            data["folder_ids"] = JsonValue{json::array_t{}};
            *out = data;
        }
        return true;
    }

    auto it = find_it();
    const int64_t now = NowMs();
    if (it == lib.items.end())
    {
        R18LibraryItem item;
        item.source_id = sid;
        item.comic_id = cid;
        item.title = title;
        item.cover = cover;
        item.author = author;
        item.subtitle = subtitle;
        item.folder_ids = SanitizeFolderIdsLocked(lib, folder_ids);
        item.favorited_at_ms = now;
        item.updated_at_ms = now;
        lib.items.push_back(std::move(item));
        it = std::prev(lib.items.end());
    }
    else
    {
        if (!title.empty())
            it->title = title;
        if (!cover.empty())
            it->cover = cover;
        if (!author.empty())
            it->author = author;
        if (!subtitle.empty())
            it->subtitle = subtitle;
        if (!folder_ids.empty())
            it->folder_ids = SanitizeFolderIdsLocked(lib, folder_ids);
        else if (it->folder_ids.empty())
            it->folder_ids = {kDefaultFolderId};
        it->updated_at_ms = now;
    }

    SaveUidLocked(uid);
    if (out)
        *out = ItemToJson(*it);
    return true;
}

bool R18LibraryStore::AssignFolders(int uid,
                                    const std::string& source_id,
                                    const std::string& comic_id,
                                    const std::vector<std::string>& folder_ids,
                                    JsonValue* out,
                                    std::string* error)
{
    const std::string sid = Trim(source_id);
    const std::string cid = Trim(comic_id);
    if (sid.empty() || cid.empty())
    {
        if (error)
            *error = "source_id and comic_id are required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& lib = by_uid_[uid];
    auto it = std::find_if(lib.items.begin(),
                           lib.items.end(),
                           [&](const R18LibraryItem& item)
                           {
                               return item.source_id == sid && item.comic_id == cid;
                           });
    if (it == lib.items.end())
    {
        if (error)
            *error = "comic is not favorited";
        return false;
    }
    it->folder_ids = SanitizeFolderIdsLocked(lib, folder_ids);
    it->updated_at_ms = NowMs();
    SaveUidLocked(uid);
    if (out)
        *out = ItemToJson(*it);
    return true;
}

bool R18LibraryStore::CreateFolder(int uid, const std::string& name, JsonValue* out, std::string* error)
{
    const std::string trimmed = Trim(name);
    if (trimmed.empty())
    {
        if (error)
            *error = "folder name is required";
        return false;
    }
    if (trimmed.size() > 64)
    {
        if (error)
            *error = "folder name is too long";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& lib = by_uid_[uid];
    EnsureDefaultFolderLocked(lib);
    for (const auto& folder : lib.folders)
    {
        if (folder.name == trimmed)
        {
            if (error)
                *error = "folder name already exists";
            return false;
        }
    }
    if (lib.folders.size() >= 100)
    {
        if (error)
            *error = "too many folders";
        return false;
    }

    R18LibraryFolder folder;
    folder.id = MakeFolderIdLocked();
    folder.name = trimmed;
    folder.created_at_ms = NowMs();
    folder.updated_at_ms = folder.created_at_ms;
    lib.folders.push_back(folder);
    SaveUidLocked(uid);
    if (out)
        *out = FolderToJson(folder);
    return true;
}

bool R18LibraryStore::RenameFolder(int uid,
                                   const std::string& folder_id,
                                   const std::string& name,
                                   JsonValue* out,
                                   std::string* error)
{
    const std::string fid = Trim(folder_id);
    const std::string trimmed = Trim(name);
    if (fid.empty() || trimmed.empty())
    {
        if (error)
            *error = "folder_id and name are required";
        return false;
    }
    if (fid == kDefaultFolderId)
    {
        if (error)
            *error = "default folder cannot be renamed";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& lib = by_uid_[uid];
    for (const auto& folder : lib.folders)
    {
        if (folder.id != fid && folder.name == trimmed)
        {
            if (error)
                *error = "folder name already exists";
            return false;
        }
    }
    auto it = std::find_if(lib.folders.begin(),
                           lib.folders.end(),
                           [&](const R18LibraryFolder& f)
                           {
                               return f.id == fid;
                           });
    if (it == lib.folders.end())
    {
        if (error)
            *error = "folder not found";
        return false;
    }
    it->name = trimmed;
    it->updated_at_ms = NowMs();
    SaveUidLocked(uid);
    if (out)
        *out = FolderToJson(*it);
    return true;
}

bool R18LibraryStore::DeleteFolder(int uid, const std::string& folder_id, JsonValue* out, std::string* error)
{
    const std::string fid = Trim(folder_id);
    if (fid.empty())
    {
        if (error)
            *error = "folder_id is required";
        return false;
    }
    if (fid == kDefaultFolderId)
    {
        if (error)
            *error = "default folder cannot be deleted";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& lib = by_uid_[uid];
    auto it = std::find_if(lib.folders.begin(),
                           lib.folders.end(),
                           [&](const R18LibraryFolder& f)
                           {
                               return f.id == fid;
                           });
    if (it == lib.folders.end())
    {
        if (error)
            *error = "folder not found";
        return false;
    }
    lib.folders.erase(it);
    for (auto& item : lib.items)
    {
        item.folder_ids.erase(std::remove(item.folder_ids.begin(), item.folder_ids.end(), fid), item.folder_ids.end());
        if (item.folder_ids.empty())
            item.folder_ids.push_back(kDefaultFolderId);
    }
    SaveUidLocked(uid);
    if (out)
        *out = LibraryToJson(lib);
    return true;
}

} // namespace memochat::r18
