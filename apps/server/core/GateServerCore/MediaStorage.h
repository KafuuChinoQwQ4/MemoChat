#pragma once

#include <filesystem>
#include <string>
#include <vector>

class IMediaStorage {
public:
    virtual ~IMediaStorage() = default;

    virtual bool StoreMergedFile(const std::string& media_type,
                                 const std::string& media_key,
                                 const std::string& origin_file_name,
                                 const std::filesystem::path& merged_file,
                                 std::string& out_storage_path,
                                 std::string& error_text) = 0;

    virtual bool ResolveReadPath(const std::string& storage_path,
                                 std::filesystem::path& out_path) const = 0;

    virtual bool ResolvePublicUrl(const std::string& storage_path,
                                  const std::string& media_type,
                                  std::string& out_url) const = 0;

    virtual bool ReadObject(const std::string& storage_path,
                            const std::string& media_type,
                            std::vector<char>& out_data,
                            std::string& out_content_type,
                            std::string& error_text) = 0;
};

class LocalMediaStorage final : public IMediaStorage {
public:
    explicit LocalMediaStorage(const std::filesystem::path& uploads_root = {});

    bool StoreMergedFile(const std::string& media_type,
                         const std::string& media_key,
                         const std::string& origin_file_name,
                         const std::filesystem::path& merged_file,
                         std::string& out_storage_path,
                         std::string& error_text) override;

    bool ResolveReadPath(const std::string& storage_path,
                         std::filesystem::path& out_path) const override;

    bool ResolvePublicUrl(const std::string& storage_path,
                          const std::string& media_type,
                          std::string& out_url) const override;

    bool ReadObject(const std::string& storage_path,
                    const std::string& media_type,
                    std::vector<char>& out_data,
                    std::string& out_content_type,
                    std::string& error_text) override;

private:
    std::filesystem::path _uploads_root;
    std::string _public_base_url;
};

IMediaStorage& GetMediaStorage();
