#include "MediaStorage.hpp"
#include "S3MediaStorage.hpp"

#include "ConfigMgr.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

import memochat.media.config_algorithms;
import memochat.media.local_storage_algorithms;

namespace
{
namespace local_modules = memochat::media::local_storage::modules;

std::filesystem::path ResolveUploadsRoot()
{
    const auto root = ConfigMgr::Inst().GetValue("Media", "RootPath");
    if (!root.empty())
    {
        return std::filesystem::path(root);
    }
    return std::filesystem::current_path() / local_modules::DefaultUploadsDirName();
}

bool LooksLikeHttpUrl(const std::string& value)
{
    return local_modules::LooksLikeHttpUrl(value.rfind("http://", 0) == 0, value.rfind("https://", 0) == 0);
}

bool HasParentTraversalSegment(const std::filesystem::path& path)
{
    for (const auto& part : path)
    {
        if (local_modules::IsParentTraversalSegment(part == ".."))
        {
            return true;
        }
    }
    return false;
}

std::filesystem::path CanonicalOrNormal(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::path resolved = std::filesystem::weakly_canonical(path, ec);
    if (!ec)
    {
        return resolved.lexically_normal();
    }

    ec.clear();
    resolved = std::filesystem::absolute(path, ec);
    if (!ec)
    {
        return resolved.lexically_normal();
    }
    return path.lexically_normal();
}

bool IsPathInsideRoot(const std::filesystem::path& candidate, const std::filesystem::path& root)
{
    std::error_code ec;
    const std::filesystem::path relative = std::filesystem::relative(candidate, root, ec);
    if (ec || relative.empty() || relative.is_absolute())
    {
        return false;
    }
    return !HasParentTraversalSegment(relative);
}

std::string GuessContentTypeLocal(const std::string& fileName, const std::string& mimeHint)
{
    if (!mimeHint.empty())
        return mimeHint;
    static const std::unordered_map<std::string, std::string> extMap = {{".png", "image/png"},
                                                                        {".jpg", "image/jpeg"},
                                                                        {".jpeg", "image/jpeg"},
                                                                        {".webp", "image/webp"},
                                                                        {".bmp", "image/bmp"},
                                                                        {".gif", "image/gif"},
                                                                        {".txt", "text/plain"},
                                                                        {".pdf", "application/pdf"}};
    const auto pos = fileName.rfind('.');
    if (pos != std::string::npos)
    {
        std::string ext = fileName.substr(pos);
        memochat::media::modules::LowerAsciiInPlace(ext.data(), ext.size());
        auto it = extMap.find(ext);
        if (it != extMap.end())
            return it->second;
    }
    return local_modules::DefaultContentType();
}

std::string BuildDateTag()
{
    using clock = std::chrono::system_clock;
    const std::time_t now = clock::to_time_t(clock::now());
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now);
#else
    localtime_r(&now, &local_tm);
#endif

    char buf[16] = {0};
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d", local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday);
    return std::string(buf);
}

std::string SanitizeFileNameForStorage(const std::string& file_name)
{
    std::string safe;
    safe.reserve(file_name.size());
    for (char c : file_name)
    {
        if (local_modules::IsAllowedStorageNameChar(static_cast<unsigned char>(c)))
        {
            safe.push_back(c);
        }
        else
        {
            safe.push_back(local_modules::SanitizedReplacementChar());
        }
    }
    if (safe.empty())
    {
        return local_modules::EmptyNameFallback();
    }
    if (local_modules::ShouldTruncateSanitizedName(safe.size()))
    {
        safe = safe.substr(safe.size() - static_cast<std::size_t>(local_modules::MaxSanitizedNameLength()));
    }
    return safe;
}
} // namespace

LocalMediaStorage::LocalMediaStorage(const std::filesystem::path& uploads_root)
{
    _uploads_root = uploads_root.empty() ? ResolveUploadsRoot() : uploads_root;
    _public_base_url = ConfigMgr::Inst().GetValue("Media", "PublicBaseUrl");
    const std::string allow_redirect = ConfigMgr::Inst().GetValue("Media", "AllowPublicRedirect");
    _allow_public_redirect = local_modules::IsPublicRedirectAllowed(allow_redirect == "true", allow_redirect == "1");
}

bool LocalMediaStorage::StoreMergedFile(const std::string& media_type,
                                        const std::string& media_key,
                                        const std::string& origin_file_name,
                                        const std::filesystem::path& merged_file,
                                        std::string& out_storage_path,
                                        std::string& error_text)
{
    if (media_key.empty())
    {
        error_text = "media key is empty";
        return false;
    }

    (void) media_type;

    if (!std::filesystem::exists(merged_file))
    {
        error_text = "merged temp file not found";
        return false;
    }

    const std::string date_tag = BuildDateTag();
    std::filesystem::path target_dir = _uploads_root / "assets" / date_tag;
    std::error_code ec;
    std::filesystem::create_directories(target_dir, ec);
    if (ec)
    {
        error_text = "create assets dir failed";
        return false;
    }

    const std::string safe_name = SanitizeFileNameForStorage(origin_file_name);
    const std::string target_name = media_key + "_" + safe_name;
    std::filesystem::path target_path = target_dir / target_name;

    std::filesystem::rename(merged_file, target_path, ec);
    if (ec)
    {
        ec.clear();
        std::filesystem::copy_file(merged_file, target_path, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            error_text = "persist media file failed";
            return false;
        }
        std::filesystem::remove(merged_file, ec);
    }

    std::filesystem::path relative = std::filesystem::path("uploads") / "assets" / date_tag / target_name;
    out_storage_path = relative.generic_string();
    return true;
}

bool LocalMediaStorage::ResolveReadPath(const std::string& storage_path, std::filesystem::path& out_path) const
{
    if (storage_path.empty())
    {
        return false;
    }

    if (LooksLikeHttpUrl(storage_path))
    {
        return false;
    }

    std::filesystem::path path(storage_path);
    if (path.is_absolute())
    {
        return false;
    }

    if (HasParentTraversalSegment(path))
    {
        return false;
    }

    std::filesystem::path relative_path = path;
    const auto storage_str = path.generic_string();
    const std::string uploads_prefix = local_modules::UploadsPathPrefix();
    if (storage_str.rfind(uploads_prefix, 0) == 0)
    {
        relative_path = std::filesystem::path(storage_str.substr(uploads_prefix.size()));
        if (relative_path.empty() || HasParentTraversalSegment(relative_path))
        {
            return false;
        }
    }

    const std::filesystem::path root = CanonicalOrNormal(_uploads_root);
    const std::filesystem::path candidate = CanonicalOrNormal(_uploads_root / relative_path);
    if (!IsPathInsideRoot(candidate, root))
    {
        return false;
    }

    out_path = candidate;
    return true;
}

bool LocalMediaStorage::ResolvePublicUrl(const std::string& storage_path,
                                         const std::string& media_type,
                                         std::string& out_url) const
{
    (void) media_type;
    if (storage_path.empty())
    {
        return false;
    }

    if (!_allow_public_redirect)
    {
        return false;
    }

    if (LooksLikeHttpUrl(storage_path))
    {
        out_url = storage_path;
        return true;
    }

    if (_public_base_url.empty())
    {
        return false;
    }

    std::string suffix = storage_path;
    if (!suffix.empty() && suffix.front() != '/')
    {
        suffix = "/" + suffix;
    }
    out_url = _public_base_url + suffix;
    return true;
}

bool LocalMediaStorage::ReadObject(const std::string& storage_path,
                                   const std::string& media_type,
                                   std::vector<char>& out_data,
                                   std::string& out_content_type,
                                   std::string& error_text)
{
    (void) media_type;
    out_data.clear();
    out_content_type = "application/octet-stream";

    std::filesystem::path full_path;
    if (!ResolveReadPath(storage_path, full_path))
    {
        error_text = "failed to resolve local read path";
        return false;
    }
    if (!std::filesystem::exists(full_path))
    {
        error_text = "file does not exist";
        return false;
    }

    std::ifstream ifs(full_path, std::ios::binary);
    if (!ifs)
    {
        error_text = "failed to open file";
        return false;
    }
    out_data.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    ifs.close();

    out_content_type = GuessContentTypeLocal(full_path.filename().string(), "");
    return true;
}

IMediaStorage& GetMediaStorage()
{
    static LocalMediaStorage local_instance;
    static S3MediaStorage s3_instance;
    static bool inited = false;
    static std::string provider;
    if (!inited)
    {
        provider = ConfigMgr::Inst().GetValue("Media", "StorageProvider");
        memochat::media::modules::LowerAsciiInPlace(provider.data(), provider.size());
        inited = true;
    }
    if (local_modules::IsS3Provider(provider == "s3", provider == "minio"))
    {
        return s3_instance;
    }
    return local_instance;
}
