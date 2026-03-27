#pragma once

#include "MediaStorage.h"

#include <memory>
#include <string>

namespace Aws {
namespace S3 {
class S3Client;
}  // namespace S3
}  // namespace Aws

class S3MediaStorage final : public IMediaStorage {
public:
    explicit S3MediaStorage();
    ~S3MediaStorage() override;

    bool StoreMergedFile(const std::string& media_key,
                         const std::string& origin_file_name,
                         const std::filesystem::path& merged_file,
                         std::string& out_storage_path,
                         std::string& error_text) override;

    bool ResolveReadPath(const std::string& storage_path,
                         std::filesystem::path& out_path) const override;

    bool ResolvePublicUrl(const std::string& storage_path,
                           std::string& out_url) const override;

private:
    bool SelectBucket(const std::string& media_type,
                      const std::string& media_key,
                      std::string& out_bucket,
                      std::string& error_text) const;

    std::string SanitizeName(const std::string& name) const;

    std::unique_ptr<Aws::S3::S3Client> _s3_client;
    std::string _region;
    std::string _public_url;
    std::string _bucket_avatar;
    std::string _bucket_file;
    std::string _bucket_image;
    std::string _bucket_video;
    bool _enabled = false;
};
