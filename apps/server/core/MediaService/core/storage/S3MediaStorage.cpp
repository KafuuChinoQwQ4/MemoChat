#include "S3MediaStorage.hpp"

#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <utility>
#include <vector>

import memochat.media.s3_storage_algorithms;

namespace
{

namespace s3_modules = memochat::media::s3_storage::modules;

std::string GetEnvOrDefault(const char* key, const char* fallback = "")
{
    const char* val = std::getenv(key);
    return val ? std::string(val) : std::string(fallback);
}

std::string TrimAscii(std::string value)
{
    auto is_space = [](unsigned char c)
    {
        return std::isspace(c) != 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(),
                             value.end(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             })
                    .base(),
                value.end());
    return value;
}

std::string LowerAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return value;
}

bool IsFalseConfigValue(std::string value)
{
    value = LowerAscii(TrimAscii(std::move(value)));
    return value == "false" || value == "0" || value == "no";
}

std::string FirstNonEmptyEnv(std::initializer_list<const char*> keys)
{
    for (const char* key : keys)
    {
        const std::string value = TrimAscii(GetEnvOrDefault(key));
        if (!value.empty())
        {
            return value;
        }
    }
    return {};
}

std::string ConfigCredentialOrEnv(std::string config_value, std::initializer_list<const char*> env_keys)
{
    config_value = TrimAscii(std::move(config_value));
    if (!config_value.empty())
    {
        return config_value;
    }
    return FirstNonEmptyEnv(env_keys);
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

std::string SanitizeForS3(const std::string& name)
{
    std::string safe;
    safe.reserve(name.size());
    for (char c : name)
    {
        if (s3_modules::IsAllowedS3NameChar(static_cast<unsigned char>(c)))
        {
            safe.push_back(c);
        }
        else
        {
            safe.push_back(s3_modules::SanitizedReplacementChar());
        }
    }
    if (safe.empty())
        return s3_modules::EmptyNameFallback();
    if (s3_modules::ShouldTruncateSanitizedName(safe.size()))
        safe = safe.substr(safe.size() - static_cast<std::size_t>(s3_modules::MaxSanitizedNameLength()));
    return safe;
}

std::string TrimMediaType(const std::string& media_type)
{
    std::string t;
    t.reserve(media_type.size());
    for (char c : media_type)
    {
        t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return t;
}

} // namespace

S3MediaStorage::S3MediaStorage()
{
    auto minio = ConfigMgr::Inst()["MinIO"];
    _region = minio["Region"].empty() ? s3_modules::DefaultRegion() : minio["Region"];
    _public_url = minio["PublicUrl"];
    _bucket_avatar = minio["BucketAvatar"];
    _bucket_file = minio["BucketFile"];
    _bucket_image = minio["BucketImage"];
    _bucket_video = minio["BucketVideo"];
    _bucket_moments = minio["BucketMoments"];
    if (_bucket_moments.empty())
    {
        _bucket_moments = s3_modules::DefaultMomentsBucket();
    }

    const std::string enabled_str = minio["Enabled"];
    const bool configured_enabled = s3_modules::IsEnabledConfigValue(enabled_str == "true", enabled_str == "1");
    const std::string allow_redirect = minio["AllowPublicRedirect"];
    _allow_public_redirect = s3_modules::IsPublicRedirectAllowed(allow_redirect == "true", allow_redirect == "1");

    if (!configured_enabled)
    {
        _startup_error = "MinIO.Enabled must be true when Media.StorageProvider is s3";
        memolog::LogWarn("s3.init", _startup_error);
        return;
    }

    const std::string endpoint = TrimAscii(minio["Endpoint"]);
    if (endpoint.empty())
    {
        _startup_error = "MinIO.Endpoint is required when Media.StorageProvider is s3";
        memolog::LogWarn("s3.init", _startup_error);
        return;
    }
    if (_bucket_avatar.empty() || _bucket_file.empty() || _bucket_image.empty() || _bucket_video.empty() ||
        _bucket_moments.empty())
    {
        _startup_error = "all MinIO bucket names are required when Media.StorageProvider is s3";
        memolog::LogWarn("s3.init", _startup_error);
        return;
    }

    const std::string access_key = ConfigCredentialOrEnv(minio["AccessKey"],
                                                         {
                                                             "MEMOCHAT_MINIO_ACCESSKEY",
                                                             "MEMOCHAT_MINIO_ACCESS_KEY",
                                                             "MEMOCHAT_MINIO_ROOT_USER",
                                                             "MINIO_ROOT_USER",
                                                             "MINIO_ACCESS_KEY",
                                                         });
    const std::string secret_key = ConfigCredentialOrEnv(minio["SecretKey"],
                                                         {
                                                             "MEMOCHAT_MINIO_SECRETKEY",
                                                             "MEMOCHAT_MINIO_SECRET_KEY",
                                                             "MEMOCHAT_MINIO_ROOT_PASSWORD",
                                                             "MINIO_ROOT_PASSWORD",
                                                             "MINIO_SECRET_KEY",
                                                         });

    if (access_key.empty() || secret_key.empty())
    {
        _startup_error = "MinIO credentials are required when Media.StorageProvider is s3";
        memolog::LogWarn("s3.init", _startup_error);
        return;
    }

    Aws::Client::ClientConfiguration config;
    config.endpointOverride = endpoint;
    config.region = _region;
    const std::string scheme = LowerAscii(TrimAscii(minio["Scheme"]));
    const bool use_https = scheme != "http";
    config.scheme = use_https ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
    config.verifySSL = use_https && !IsFalseConfigValue(minio["VerifySSL"]);

    Aws::Auth::AWSCredentials credentials(access_key, secret_key);

    _s3_client = std::make_unique<Aws::S3::S3Client>(credentials,
                                                     config,
                                                     Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
                                                     false);
    if (!_s3_client)
    {
        _startup_error = "failed to create S3 client";
        memolog::LogWarn("s3.init", _startup_error);
        return;
    }

    std::vector<std::string> checked_buckets;
    for (const auto& bucket : {_bucket_avatar, _bucket_file, _bucket_image, _bucket_video, _bucket_moments})
    {
        if (std::find(checked_buckets.begin(), checked_buckets.end(), bucket) != checked_buckets.end())
        {
            continue;
        }
        checked_buckets.push_back(bucket);
        Aws::S3::Model::HeadBucketRequest request;
        request.SetBucket(Aws::String(bucket));
        const auto outcome = _s3_client->HeadBucket(request);
        if (!outcome.IsSuccess())
        {
            _startup_error = "MinIO bucket readiness check failed for " + bucket + ": " +
                             std::string(outcome.GetError().GetMessage().c_str());
            memolog::LogWarn("s3.init", _startup_error);
            _s3_client.reset();
            return;
        }
    }
    _enabled = true;

    memolog::LogInfo("s3.init.ok",
                     "S3MediaStorage initialized, endpoint=" + minio["Endpoint"] + " bucket_avatar=" + _bucket_avatar +
                         " bucket_file=" + _bucket_file + " bucket_image=" + _bucket_image +
                         " bucket_video=" + _bucket_video + " bucket_moments=" + _bucket_moments);
}

S3MediaStorage::~S3MediaStorage() = default;

bool S3MediaStorage::Ready() const noexcept
{
    return _enabled && _s3_client != nullptr && _startup_error.empty();
}

const std::string& S3MediaStorage::StartupError() const noexcept
{
    return _startup_error;
}

bool S3MediaStorage::SelectBucket(const std::string& media_type,
                                  const std::string& media_key,
                                  std::string& out_bucket,
                                  std::string& error_text) const
{
    (void) media_key;
    std::string t = TrimMediaType(media_type);
    if (t == "avatar")
    {
        out_bucket = _bucket_avatar;
    }
    else if (t == "moment_image" || t == "moment_video")
    {
        out_bucket = _bucket_moments;
    }
    else if (t == "image")
    {
        out_bucket = _bucket_image;
    }
    else if (t == "video" || t == "audio")
    {
        out_bucket = _bucket_video;
    }
    else
    {
        out_bucket = _bucket_file;
    }
    return true;
}

std::string S3MediaStorage::SanitizeName(const std::string& name) const
{
    return SanitizeForS3(name);
}

bool S3MediaStorage::StoreMergedFile(const std::string& media_type,
                                     const std::string& media_key,
                                     const std::string& origin_file_name,
                                     const std::filesystem::path& merged_file,
                                     std::string& out_storage_path,
                                     std::string& error_text)
{
    if (!_enabled || !_s3_client)
    {
        error_text = "S3MediaStorage is not enabled or credentials missing";
        return false;
    }

    if (media_key.empty())
    {
        error_text = "media key is empty";
        return false;
    }

    std::error_code filesystem_error;
    if (!std::filesystem::exists(merged_file, filesystem_error) || filesystem_error)
    {
        error_text = "merged temp file not found";
        return false;
    }

    std::string bucket;
    if (!SelectBucket(media_type, media_key, bucket, error_text))
    {
        return false;
    }

    const std::string date_tag = BuildDateTag();
    const std::string safe_name = SanitizeName(origin_file_name);
    const std::string object_key = s3_modules::AssetKeyPrefix() + date_tag + "/" + media_key + "_" + safe_name;

    std::ifstream ifs(merged_file, std::ios::binary);
    if (!ifs.is_open())
    {
        error_text = "open merged file for reading failed";
        return false;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    const auto& data_str = oss.str();
    ifs.close();

    Aws::String aws_bucket(bucket);
    Aws::String aws_key(object_key);
    Aws::String aws_data(data_str.c_str(), data_str.size());

    Aws::S3::Model::PutObjectRequest put_request;
    put_request.WithBucket(aws_bucket).WithKey(aws_key).SetBody(std::make_shared<Aws::StringStream>(aws_data));
    put_request.SetContentType(s3_modules::DefaultObjectContentType());

    auto put_outcome = _s3_client->PutObject(put_request);
    if (!put_outcome.IsSuccess())
    {
        const auto& err = put_outcome.GetError();
        error_text = "S3 PutObject failed: " + std::string(err.GetMessage().c_str());
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(merged_file, ec);

    // Path-style MinIO/S3 public URL is {endpoint}/{bucket}/{key}; persist bucket so redirects work.
    out_storage_path = bucket + "/" + object_key;

    memolog::LogInfo("s3.store_merged.ok",
                     "S3MediaStorage::StoreMergedFile succeeded, bucket=" + bucket + " media_type=" + media_type +
                         " size=" + std::to_string(data_str.size()));
    return true;
}

bool S3MediaStorage::StoragePathHasConfiguredBucketPrefix(const std::string& storage_path) const
{
    const size_t slash = storage_path.find('/');
    if (!s3_modules::HasLeadingBucketSegment(slash != std::string::npos, slash))
    {
        return false;
    }
    const std::string first = storage_path.substr(0, slash);
    return first == _bucket_avatar || first == _bucket_file || first == _bucket_image || first == _bucket_video ||
           first == _bucket_moments;
}

bool S3MediaStorage::ResolveReadPath(const std::string& storage_path, std::filesystem::path& out_path) const
{
    (void) storage_path;
    (void) out_path;
    return false;
}

bool S3MediaStorage::ResolvePublicUrl(const std::string& storage_path,
                                      const std::string& media_type,
                                      std::string& out_url) const
{
    if (storage_path.empty())
        return false;
    if (!_allow_public_redirect)
        return false;
    if (_public_url.empty())
        return false;

    std::string path_for_url = storage_path;
    if (!StoragePathHasConfiguredBucketPrefix(path_for_url))
    {
        std::string bucket;
        std::string err;
        if (!SelectBucket(media_type, "", bucket, err) || bucket.empty())
        {
            return false;
        }
        path_for_url = bucket + "/" + storage_path;
    }

    if (_public_url.back() == '/')
    {
        out_url = _public_url + path_for_url;
    }
    else
    {
        out_url = _public_url + "/" + path_for_url;
    }
    return true;
}

bool S3MediaStorage::ReadObject(const std::string& storage_path,
                                const std::string& media_type,
                                std::vector<char>& out_data,
                                std::string& out_content_type,
                                std::string& error_text)
{
    out_data.clear();
    out_content_type = s3_modules::DefaultObjectContentType();

    if (!_enabled || !_s3_client)
    {
        error_text = "S3MediaStorage not enabled";
        return false;
    }

    std::string path_for_bucket = storage_path;
    std::string bucket;
    if (StoragePathHasConfiguredBucketPrefix(path_for_bucket))
    {
        const size_t slash = path_for_bucket.find('/');
        bucket = path_for_bucket.substr(0, slash);
        path_for_bucket = path_for_bucket.substr(slash + 1);
    }
    else
    {
        if (!SelectBucket(media_type, "", bucket, error_text))
        {
            error_text += " [ReadObject SelectBucket failed, media_type=" + media_type + "]";
            return false;
        }
    }

    memolog::LogInfo("s3.read_object", "S3MediaStorage::ReadObject calling GetObject bucket=" + bucket);

    Aws::S3::Model::GetObjectRequest get_request;
    get_request.SetBucket(Aws::String(bucket));
    get_request.SetKey(Aws::String(path_for_bucket));

    auto get_outcome = _s3_client->GetObject(get_request);
    if (!get_outcome.IsSuccess())
    {
        const auto& err = get_outcome.GetError();
        error_text = "S3 GetObject failed: bucket=" + bucket + " err=" + std::string(err.GetMessage().c_str());
        return false;
    }

    const auto& result = get_outcome.GetResult();
    if (result.GetContentLength() > 0)
    {
        const auto& ct = result.GetContentType();
        if (!ct.empty())
        {
            out_content_type = std::string(ct.c_str(), ct.size());
        }
        auto& body = result.GetBody();
        out_data.assign(std::istreambuf_iterator<char>(body), std::istreambuf_iterator<char>());
    }

    memolog::LogInfo("s3.read_object.ok",
                     "S3MediaStorage::ReadObject success, bucket=" + bucket + " content_length=" +
                         std::to_string(result.GetContentLength()) + " content_type=" + out_content_type);
    return true;
}
