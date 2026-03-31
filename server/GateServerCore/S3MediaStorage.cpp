#include "S3MediaStorage.h"

#include "ConfigMgr.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <sstream>

namespace {

std::string GetEnvOrDefault(const char* key, const char* fallback = "") {
    const char* val = std::getenv(key);
    return val ? std::string(val) : std::string(fallback);
}

std::string BuildDateTag() {
    using clock = std::chrono::system_clock;
    const std::time_t now = clock::to_time_t(clock::now());
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now);
#else
    localtime_r(&now, &local_tm);
#endif
    char buf[16] = {0};
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d",
                  local_tm.tm_year + 1900,
                  local_tm.tm_mon + 1,
                  local_tm.tm_mday);
    return std::string(buf);
}

std::string SanitizeForS3(const std::string& name) {
    std::string safe;
    safe.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
            safe.push_back(c);
        } else {
            safe.push_back('_');
        }
    }
    if (safe.empty()) return "file.bin";
    if (safe.size() > 96) safe = safe.substr(safe.size() - 96);
    return safe;
}

std::string TrimMediaType(const std::string& media_type) {
    std::string t;
    t.reserve(media_type.size());
    for (char c : media_type) {
        t.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return t;
}

}  // namespace

S3MediaStorage::S3MediaStorage() {
    auto minio = ConfigMgr::Inst()["MinIO"];
    _region       = minio["Region"].empty() ? "us-east-1" : minio["Region"];
    _public_url   = minio["PublicUrl"];
    _bucket_avatar = minio["BucketAvatar"];
    _bucket_file   = minio["BucketFile"];
    _bucket_image  = minio["BucketImage"];
    _bucket_video  = minio["BucketVideo"];

    const std::string enabled_str = minio["Enabled"];
    _enabled = (enabled_str == "true" || enabled_str == "1");

    if (!_enabled) return;

    const std::string access_key = minio["AccessKey"].empty()
        ? GetEnvOrDefault("MINIO_ACCESS_KEY")
        : minio["AccessKey"];
    const std::string secret_key = minio["SecretKey"].empty()
        ? GetEnvOrDefault("MINIO_SECRET_KEY")
        : minio["SecretKey"];

    if (access_key.empty() || secret_key.empty()) {
        return;
    }

    Aws::Client::ClientConfiguration config;
    config.endpointOverride = minio["Endpoint"];
    config.region = _region;
    config.scheme = Aws::Http::Scheme::HTTP;
    config.verifySSL = false;

    Aws::Auth::AWSCredentials credentials(access_key, secret_key);

    _s3_client = std::make_unique<Aws::S3::S3Client>(
        credentials, config,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
        false);
}

S3MediaStorage::~S3MediaStorage() = default;

bool S3MediaStorage::SelectBucket(const std::string& media_type,
                                   const std::string& media_key,
                                   std::string& out_bucket,
                                   std::string& error_text) const {
    (void)media_key;
    std::string t = TrimMediaType(media_type);
    if (t == "avatar") {
        out_bucket = _bucket_avatar;
    } else if (t == "image") {
        out_bucket = _bucket_image;
    } else if (t == "video" || t == "audio") {
        out_bucket = _bucket_video;
    } else {
        out_bucket = _bucket_file;
    }
    return true;
}

std::string S3MediaStorage::SanitizeName(const std::string& name) const {
    return SanitizeForS3(name);
}

bool S3MediaStorage::StoreMergedFile(const std::string& media_key,
                                       const std::string& origin_file_name,
                                       const std::filesystem::path& merged_file,
                                       std::string& out_storage_path,
                                       std::string& error_text) {
    if (!_enabled || !_s3_client) {
        error_text = "S3MediaStorage is not enabled or credentials missing";
        return false;
    }

    if (media_key.empty()) {
        error_text = "media key is empty";
        return false;
    }

    if (!std::filesystem::exists(merged_file)) {
        error_text = "merged temp file not found";
        return false;
    }

    std::string bucket;
    if (!SelectBucket("", media_key, bucket, error_text)) {
        return false;
    }

    const std::string date_tag = BuildDateTag();
    const std::string safe_name = SanitizeName(origin_file_name);
    const std::string object_key = "assets/" + date_tag + "/" + media_key + "_" + safe_name;

    std::ifstream ifs(merged_file, std::ios::binary);
    if (!ifs.is_open()) {
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
    put_request.WithBucket(aws_bucket).WithKey(aws_key).SetBody(
        std::make_shared<Aws::StringStream>(aws_data));
    put_request.SetContentType("application/octet-stream");

    auto put_outcome = _s3_client->PutObject(put_request);
    if (!put_outcome.IsSuccess()) {
        const auto& err = put_outcome.GetError();
        error_text = "S3 PutObject failed: " + std::string(err.GetMessage().c_str());
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(merged_file, ec);

    out_storage_path = object_key;
    return true;
}

bool S3MediaStorage::ResolveReadPath(const std::string& storage_path,
                                       std::filesystem::path& out_path) const {
    (void)storage_path;
    (void)out_path;
    return false;
}

bool S3MediaStorage::ResolvePublicUrl(const std::string& storage_path,
                                        std::string& out_url) const {
    if (storage_path.empty()) return false;
    if (_public_url.empty()) return false;

    if (_public_url.back() == '/') {
        out_url = _public_url + storage_path;
    } else {
        out_url = _public_url + "/" + storage_path;
    }
    return true;
}
