#include "S3MediaStorage.h"

#include "ConfigMgr.h"
#include "logging/Logger.h"

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

    if (!_enabled) {
        memolog::LogWarn("s3.init", "S3MediaStorage disabled (Enabled != true)");
        return;
    }

    const std::string access_key = minio["AccessKey"].empty()
        ? GetEnvOrDefault("MINIO_ACCESS_KEY")
        : minio["AccessKey"];
    const std::string secret_key = minio["SecretKey"].empty()
        ? GetEnvOrDefault("MINIO_SECRET_KEY")
        : minio["SecretKey"];

    if (access_key.empty() || secret_key.empty()) {
        memolog::LogWarn("s3.init", "S3MediaStorage credentials empty, minio disabled");
        _enabled = false;
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

    memolog::LogInfo("s3.init.ok",
        "S3MediaStorage initialized, endpoint=" + minio["Endpoint"] +
        " bucket_avatar=" + _bucket_avatar +
        " bucket_file=" + _bucket_file +
        " bucket_image=" + _bucket_image +
        " bucket_video=" + _bucket_video);
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

bool S3MediaStorage::StoreMergedFile(const std::string& media_type,
                                       const std::string& media_key,
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
    if (!SelectBucket(media_type, media_key, bucket, error_text)) {
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

    // Path-style MinIO/S3 public URL is {endpoint}/{bucket}/{key}; persist bucket so redirects work.
    out_storage_path = bucket + "/" + object_key;

    memolog::LogInfo("s3.store_merged.ok",
        "S3MediaStorage::StoreMergedFile succeeded, bucket=" + bucket +
        " object_key=" + object_key +
        " storage_path=" + out_storage_path +
        " media_type=" + media_type +
        " size=" + std::to_string(data_str.size()));
    return true;
}

bool S3MediaStorage::StoragePathHasConfiguredBucketPrefix(const std::string& storage_path) const {
    const size_t slash = storage_path.find('/');
    if (slash == std::string::npos || slash == 0) {
        return false;
    }
    const std::string first = storage_path.substr(0, slash);
    return first == _bucket_avatar || first == _bucket_file || first == _bucket_image ||
           first == _bucket_video;
}

bool S3MediaStorage::ResolveReadPath(const std::string& storage_path,
                                       std::filesystem::path& out_path) const {
    (void)storage_path;
    (void)out_path;
    return false;
}

bool S3MediaStorage::ResolvePublicUrl(const std::string& storage_path,
                                      const std::string& media_type,
                                      std::string& out_url) const {
    if (storage_path.empty()) return false;
    if (_public_url.empty()) return false;

    std::string path_for_url = storage_path;
    if (!StoragePathHasConfiguredBucketPrefix(path_for_url)) {
        std::string bucket;
        std::string err;
        if (!SelectBucket(media_type, "", bucket, err) || bucket.empty()) {
            return false;
        }
        path_for_url = bucket + "/" + storage_path;
    }

    if (_public_url.back() == '/') {
        out_url = _public_url + path_for_url;
    } else {
        out_url = _public_url + "/" + path_for_url;
    }
    return true;
}

bool S3MediaStorage::ReadObject(const std::string& storage_path,
                                const std::string& media_type,
                                std::vector<char>& out_data,
                                std::string& out_content_type,
                                std::string& error_text) {
    out_data.clear();
    out_content_type = "application/octet-stream";

    if (!_enabled || !_s3_client) {
        error_text = "S3MediaStorage not enabled";
        return false;
    }

    std::string path_for_bucket = storage_path;
    std::string bucket;
    if (StoragePathHasConfiguredBucketPrefix(path_for_bucket)) {
        const size_t slash = path_for_bucket.find('/');
        bucket = path_for_bucket.substr(0, slash);
        path_for_bucket = path_for_bucket.substr(slash + 1);
    } else {
        if (!SelectBucket(media_type, "", bucket, error_text)) {
            error_text += " [ReadObject SelectBucket failed, media_type=" + media_type + ", storage_path=" + storage_path + "]";
            return false;
        }
    }

    memolog::LogInfo("s3.read_object",
        "S3MediaStorage::ReadObject calling GetObject bucket=" + bucket + " key=" + path_for_bucket);

    Aws::S3::Model::GetObjectRequest get_request;
    get_request.SetBucket(Aws::String(bucket));
    get_request.SetKey(Aws::String(path_for_bucket));

    auto get_outcome = _s3_client->GetObject(get_request);
    if (!get_outcome.IsSuccess()) {
        const auto& err = get_outcome.GetError();
        error_text = "S3 GetObject failed: bucket=" + bucket + " key=" + path_for_bucket + " err=" + std::string(err.GetMessage().c_str());
        return false;
    }

    const auto& result = get_outcome.GetResult();
    if (result.GetContentLength() > 0) {
        const auto& ct = result.GetContentType();
        if (!ct.empty()) {
            out_content_type = std::string(ct.c_str(), ct.size());
        }
        auto& body = result.GetBody();
        out_data.assign(std::istreambuf_iterator<char>(body),
                        std::istreambuf_iterator<char>());
    }

    memolog::LogInfo("s3.read_object.ok",
        "S3MediaStorage::ReadObject success, bucket=" + bucket + " key=" + path_for_bucket +
        " content_length=" + std::to_string(result.GetContentLength()) +
        " content_type=" + out_content_type);
    return true;
}
