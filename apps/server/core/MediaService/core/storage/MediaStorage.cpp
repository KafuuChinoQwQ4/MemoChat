#include "MediaStorage.hpp"
#include "S3MediaStorage.hpp"

#include "ConfigMgr.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
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
    std::error_code filesystem_error;
    const auto current_path = std::filesystem::current_path(filesystem_error);
    return filesystem_error ? std::filesystem::path(local_modules::DefaultUploadsDirName())
                            : current_path / local_modules::DefaultUploadsDirName();
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

    std::error_code filesystem_error;
    std::filesystem::create_directories(_uploads_root, filesystem_error);
    if (filesystem_error)
    {
        _startup_error = "create media root failed: " + filesystem_error.message();
        return;
    }
    if (!std::filesystem::is_directory(_uploads_root, filesystem_error) || filesystem_error)
    {
        _startup_error = filesystem_error ? "inspect media root failed: " + filesystem_error.message()
                                          : "media root is not a directory";
        return;
    }

    const auto probe_name = ".memochat-write-probe-" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
    const auto probe_path = _uploads_root / probe_name;
    std::FILE* probe = std::fopen(probe_path.string().c_str(), "wb");
    if (probe == nullptr)
    {
        _startup_error = "media root is not writable: " + std::string(std::strerror(errno));
        return;
    }
    const unsigned char marker = 0;
    const bool wrote_probe = std::fwrite(&marker, 1, 1, probe) == 1 && std::fflush(probe) == 0;
    const int close_result = std::fclose(probe);
    if (!wrote_probe || close_result != 0)
    {
        _startup_error = "media root write probe failed: " + std::string(std::strerror(errno));
        std::filesystem::remove(probe_path, filesystem_error);
        return;
    }
    std::filesystem::remove(probe_path, filesystem_error);
}

bool LocalMediaStorage::Ready() const noexcept
{
    return _startup_error.empty();
}

const std::string& LocalMediaStorage::StartupError() const noexcept
{
    return _startup_error;
}

bool LocalMediaStorage::StoreMergedFile(const std::string& media_type,
                                        const std::string& media_key,
                                        const std::string& origin_file_name,
                                        const std::filesystem::path& merged_file,
                                        std::string& out_storage_path,
                                        std::string& error_text)
{
    if (!Ready())
    {
        error_text = _startup_error;
        return false;
    }
    if (media_key.empty())
    {
        error_text = "media key is empty";
        return false;
    }

    (void) media_type;

    std::error_code ec;
    if (!std::filesystem::exists(merged_file, ec) || ec)
    {
        error_text = "merged temp file not found";
        return false;
    }

    const std::string date_tag = BuildDateTag();
    std::filesystem::path target_dir = _uploads_root / "assets" / date_tag;
    ec.clear();
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
    if (!Ready())
    {
        return false;
    }
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
    if (!Ready())
    {
        return false;
    }
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

    if (!Ready())
    {
        error_text = _startup_error;
        return false;
    }

    std::filesystem::path full_path;
    if (!ResolveReadPath(storage_path, full_path))
    {
        error_text = "failed to resolve local read path";
        return false;
    }
    std::error_code filesystem_error;
    if (!std::filesystem::exists(full_path, filesystem_error) || filesystem_error)
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

namespace
{

class UnavailableMediaStorage final : public IMediaStorage
{
public:
    bool Ready() const noexcept override
    {
        return false;
    }

    const std::string& StartupError() const noexcept override
    {
        return error_;
    }

    void SetError(std::string error)
    {
        error_ = std::move(error);
    }

    bool StoreMergedFile(const std::string&,
                         const std::string&,
                         const std::string&,
                         const std::filesystem::path&,
                         std::string&,
                         std::string& error_text) override
    {
        error_text = error_;
        return false;
    }

    bool ResolveReadPath(const std::string&, std::filesystem::path&) const override
    {
        return false;
    }

    bool ResolvePublicUrl(const std::string&, const std::string&, std::string&) const override
    {
        return false;
    }

    bool ReadObject(const std::string&,
                    const std::string&,
                    std::vector<char>&,
                    std::string&,
                    std::string& error_text) override
    {
        error_text = error_;
        return false;
    }

private:
    std::string error_ = "media storage is not initialized";
};

class MediaStorageState
{
public:
    bool Initialize(std::string* error)
    {
        if (shut_down_)
        {
            if (error != nullptr)
            {
                *error = unavailable_.StartupError();
            }
            return false;
        }
        if (!storage_)
        {
            std::string provider = ConfigMgr::Inst().GetValue("Media", "StorageProvider");
            memochat::media::modules::LowerAsciiInPlace(provider.data(), provider.size());
            if (provider.empty() || provider == "local")
            {
                storage_ = std::make_unique<LocalMediaStorage>();
            }
            else if (local_modules::IsS3Provider(provider == "s3", provider == "minio"))
            {
                storage_ = std::make_unique<S3MediaStorage>();
            }
            else
            {
                unavailable_.SetError("unsupported Media.StorageProvider: " + provider);
            }
        }

        if (!storage_ || !storage_->Ready())
        {
            const std::string startup_error = storage_ ? storage_->StartupError() : unavailable_.StartupError();
            unavailable_.SetError(startup_error);
            if (error != nullptr)
            {
                *error = startup_error;
            }
            return false;
        }
        if (error != nullptr)
        {
            error->clear();
        }
        return true;
    }

    void Shutdown()
    {
        storage_.reset();
        unavailable_.SetError("media storage is shut down");
        shut_down_ = true;
    }

    IMediaStorage& Get()
    {
        return storage_ ? *storage_ : unavailable_;
    }

private:
    std::unique_ptr<IMediaStorage> storage_;
    UnavailableMediaStorage unavailable_;
    bool shut_down_ = false;
};

MediaStorageState& StorageState()
{
    static MediaStorageState state;
    return state;
}

} // namespace

bool InitializeMediaStorage(std::string* error)
{
    return StorageState().Initialize(error);
}

void ShutdownMediaStorage()
{
    StorageState().Shutdown();
}

IMediaStorage& GetMediaStorage()
{
    auto& state = StorageState();
    state.Initialize(nullptr);
    return state.Get();
}
