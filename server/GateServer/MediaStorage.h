#pragma once

#include <filesystem>
#include <string>

class IMediaStorage {
public:
    virtual ~IMediaStorage() = default;

    virtual bool StoreMergedFile(const std::string& media_key,
                                 const std::string& origin_file_name,
                                 const std::filesystem::path& merged_file,
                                 std::string& out_storage_path,
                                 std::string& error_text) = 0;

    virtual bool ResolveReadPath(const std::string& storage_path,
                                 std::filesystem::path& out_path) const = 0;
};

class LocalMediaStorage final : public IMediaStorage {
public:
    LocalMediaStorage();

    bool StoreMergedFile(const std::string& media_key,
                         const std::string& origin_file_name,
                         const std::filesystem::path& merged_file,
                         std::string& out_storage_path,
                         std::string& error_text) override;

    bool ResolveReadPath(const std::string& storage_path,
                         std::filesystem::path& out_path) const override;

private:
    std::filesystem::path _uploads_root;
};

