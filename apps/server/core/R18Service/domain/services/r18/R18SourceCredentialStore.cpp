#include "r18/R18SourceCredentialStore.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace memochat::r18
{
namespace
{

using json::JsonValue;

constexpr std::size_t kMasterKeyBytes = 32;
constexpr std::size_t kGcmNonceBytes = 12;
constexpr std::size_t kGcmTagBytes = 16;
constexpr int kCredentialEnvelopeVersion = 1;
constexpr std::string_view kCredentialEnvelopeAlgorithm = "AES-256-GCM";
constexpr std::string_view kCredentialMasterKeyEnvironment = "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY";

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

bool IsEhentaiFamily(const std::string& source_id)
{
    return source_id == "ehentai.official" || source_id == "exhentai.official";
}

void SetStorageError(std::string* error, const std::string& message)
{
    if (error)
        *error = message;
}

void SecureClear(std::string& value)
{
    if (!value.empty())
        OPENSSL_cleanse(value.data(), value.size());
    value.clear();
}

int HexNibble(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'F')
        return value - 'A' + 10;
    return -1;
}

bool DecodeHex(std::string_view encoded, std::vector<unsigned char>& decoded)
{
    decoded.clear();
    if ((encoded.size() % 2U) != 0U)
        return false;

    decoded.reserve(encoded.size() / 2U);
    for (std::size_t i = 0; i < encoded.size(); i += 2U)
    {
        const int high = HexNibble(encoded[i]);
        const int low = HexNibble(encoded[i + 1U]);
        if (high < 0 || low < 0)
        {
            decoded.clear();
            return false;
        }
        decoded.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return true;
}

std::string EncodeHex(const unsigned char* data, std::size_t size)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string encoded(size * 2U, '0');
    for (std::size_t i = 0; i < size; ++i)
    {
        encoded[i * 2U] = kHex[data[i] >> 4U];
        encoded[i * 2U + 1U] = kHex[data[i] & 0x0FU];
    }
    return encoded;
}

bool LoadMasterKey(std::array<unsigned char, kMasterKeyBytes>& key, std::string* error)
{
    const char* encoded = std::getenv(kCredentialMasterKeyEnvironment.data());
    if (encoded == nullptr || std::string_view(encoded).size() != kMasterKeyBytes * 2U)
    {
        SetStorageError(error, "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY must be exactly 64 hexadecimal characters");
        return false;
    }

    for (std::size_t i = 0; i < key.size(); ++i)
    {
        const int high = HexNibble(encoded[i * 2U]);
        const int low = HexNibble(encoded[i * 2U + 1U]);
        if (high < 0 || low < 0)
        {
            OPENSSL_cleanse(key.data(), key.size());
            SetStorageError(error, "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY must be exactly 64 hexadecimal characters");
            return false;
        }
        key[i] = static_cast<unsigned char>((high << 4) | low);
    }
    return true;
}

std::string CredentialAad(int uid)
{
    return "memochat:r18:credentials:v1:uid=" + std::to_string(uid);
}

bool EncryptCredentialPayload(int uid, const std::string& plaintext, JsonValue& envelope, std::string* error)
{
    if (plaintext.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        SetStorageError(error, "credential payload is too large to encrypt");
        return false;
    }

    std::array<unsigned char, kMasterKeyBytes> key{};
    if (!LoadMasterKey(key, error))
        return false;

    std::array<unsigned char, kGcmNonceBytes> nonce{};
    std::array<unsigned char, kGcmTagBytes> tag{};
    if (RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1)
    {
        OPENSSL_cleanse(key.data(), key.size());
        SetStorageError(error, "credential encryption nonce generation failed");
        return false;
    }

    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    if (context == nullptr)
    {
        OPENSSL_cleanse(key.data(), key.size());
        SetStorageError(error, "credential encryption context allocation failed");
        return false;
    }

    const std::string aad = CredentialAad(uid);
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int output_size = 0;
    int final_size = 0;
    int aad_size = 0;
    bool encrypted = EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    encrypted =
        encrypted && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) == 1;
    encrypted = encrypted && EVP_EncryptInit_ex(context, nullptr, nullptr, key.data(), nonce.data()) == 1;
    encrypted = encrypted && EVP_EncryptUpdate(context,
                                               nullptr,
                                               &aad_size,
                                               reinterpret_cast<const unsigned char*>(aad.data()),
                                               static_cast<int>(aad.size())) == 1;
    encrypted = encrypted && EVP_EncryptUpdate(context,
                                               ciphertext.data(),
                                               &output_size,
                                               reinterpret_cast<const unsigned char*>(plaintext.data()),
                                               static_cast<int>(plaintext.size())) == 1;
    encrypted = encrypted && EVP_EncryptFinal_ex(context, ciphertext.data() + output_size, &final_size) == 1;
    output_size += final_size;
    encrypted =
        encrypted && EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data()) == 1;

    EVP_CIPHER_CTX_free(context);
    OPENSSL_cleanse(key.data(), key.size());
    if (!encrypted)
    {
        OPENSSL_cleanse(ciphertext.data(), ciphertext.size());
        SetStorageError(error, "credential payload encryption failed");
        return false;
    }

    envelope = JsonValue(json::object_t{});
    envelope["version"] = kCredentialEnvelopeVersion;
    envelope["algorithm"] = std::string(kCredentialEnvelopeAlgorithm);
    envelope["nonce"] = EncodeHex(nonce.data(), nonce.size());
    envelope["ciphertext"] = EncodeHex(ciphertext.data(), static_cast<std::size_t>(output_size));
    envelope["tag"] = EncodeHex(tag.data(), tag.size());
    OPENSSL_cleanse(ciphertext.data(), ciphertext.size());
    return true;
}

bool DecryptCredentialPayload(int uid, const JsonValue& envelope, std::string& plaintext, std::string* error)
{
    std::vector<unsigned char> nonce;
    std::vector<unsigned char> ciphertext;
    std::vector<unsigned char> tag;
    if (!DecodeHex(json::glaze_safe_get<std::string>(envelope, "nonce", ""), nonce) || nonce.size() != kGcmNonceBytes ||
        !DecodeHex(json::glaze_safe_get<std::string>(envelope, "ciphertext", ""), ciphertext) || ciphertext.empty() ||
        ciphertext.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        !DecodeHex(json::glaze_safe_get<std::string>(envelope, "tag", ""), tag) || tag.size() != kGcmTagBytes)
    {
        SetStorageError(error, "credential file contains invalid encrypted fields");
        return false;
    }

    std::array<unsigned char, kMasterKeyBytes> key{};
    if (!LoadMasterKey(key, error))
        return false;

    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    if (context == nullptr)
    {
        OPENSSL_cleanse(key.data(), key.size());
        SetStorageError(error, "credential decryption context allocation failed");
        return false;
    }

    const std::string aad = CredentialAad(uid);
    std::vector<unsigned char> decrypted(ciphertext.size() + EVP_MAX_BLOCK_LENGTH);
    int output_size = 0;
    int final_size = 0;
    int aad_size = 0;
    bool decrypted_ok = EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    decrypted_ok = decrypted_ok &&
                   EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) == 1;
    decrypted_ok = decrypted_ok && EVP_DecryptInit_ex(context, nullptr, nullptr, key.data(), nonce.data()) == 1;
    decrypted_ok = decrypted_ok && EVP_DecryptUpdate(context,
                                                     nullptr,
                                                     &aad_size,
                                                     reinterpret_cast<const unsigned char*>(aad.data()),
                                                     static_cast<int>(aad.size())) == 1;
    decrypted_ok = decrypted_ok && EVP_DecryptUpdate(context,
                                                     decrypted.data(),
                                                     &output_size,
                                                     ciphertext.data(),
                                                     static_cast<int>(ciphertext.size())) == 1;
    decrypted_ok = decrypted_ok &&
                   EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data()) == 1;
    decrypted_ok = decrypted_ok && EVP_DecryptFinal_ex(context, decrypted.data() + output_size, &final_size) == 1;
    output_size += final_size;

    EVP_CIPHER_CTX_free(context);
    OPENSSL_cleanse(key.data(), key.size());
    if (!decrypted_ok)
    {
        OPENSSL_cleanse(decrypted.data(), decrypted.size());
        SetStorageError(error, "credential file authentication failed");
        return false;
    }

    plaintext.assign(reinterpret_cast<const char*>(decrypted.data()), static_cast<std::size_t>(output_size));
    OPENSSL_cleanse(decrypted.data(), decrypted.size());
    return true;
}

bool TightenCredentialFilePermissions(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace,
                                 ec);
    return !ec;
}

} // namespace

R18SourceCredentialStore& R18SourceCredentialStore::Instance()
{
    static R18SourceCredentialStore store;
    return store;
}

R18SourceCredentialStore::R18SourceCredentialStore()
{
    root_ = ResolveRoot();
    EnsureRootLocked(nullptr);
}

std::filesystem::path R18SourceCredentialStore::ResolveRoot() const
{
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec)
    {
        const auto local = cwd / "data" / "r18" / "credentials";
        if (std::filesystem::exists(local, ec) || std::filesystem::create_directories(local, ec) || !ec)
            return local;
    }
    return std::filesystem::temp_directory_path() / "memochat_r18_credentials";
}

std::filesystem::path R18SourceCredentialStore::PathForUid(int uid) const
{
    return root_ / (std::to_string(uid) + ".json");
}

bool R18SourceCredentialStore::EnsureRootLocked(std::string* error)
{
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(root_, ec);
    if (ec && ec != std::errc::no_such_file_or_directory)
    {
        SetStorageError(error, "credential storage path cannot be inspected");
        return false;
    }
    if (!ec && std::filesystem::is_symlink(status))
    {
        SetStorageError(error, "credential storage path must not be a symlink");
        return false;
    }
    if (!ec && std::filesystem::exists(status) && !std::filesystem::is_directory(status))
    {
        SetStorageError(error, "credential storage path is not a directory");
        return false;
    }

    ec.clear();
    std::filesystem::create_directories(root_, ec);
    if (ec)
    {
        SetStorageError(error, "credential storage directory cannot be created");
        return false;
    }
    std::filesystem::permissions(root_, std::filesystem::perms::owner_all, std::filesystem::perm_options::replace, ec);
    if (ec)
    {
        SetStorageError(error, "credential storage directory permissions cannot be secured");
        return false;
    }
    return true;
}

bool R18SourceCredentialStore::BackingFileMatchesLoadedSnapshotLocked(int uid, std::string* error) const
{
    const auto snapshot = loaded_envelopes_.find(uid);
    if (snapshot == loaded_envelopes_.end())
    {
        SetStorageError(error, "credential file load state is unavailable");
        return false;
    }

    const auto path = PathForUid(uid);
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(path, ec);
    const bool missing = ec == std::errc::no_such_file_or_directory || (!ec && !std::filesystem::exists(status));
    if (!snapshot->second.has_value())
    {
        if (missing)
            return true;
        SetStorageError(error, "credential file changed after it was loaded");
        return false;
    }
    if (ec || std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status))
    {
        SetStorageError(error, "credential file changed after it was loaded");
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        SetStorageError(error, "credential file cannot be rechecked before writing");
        return false;
    }
    const std::string current((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (current != *snapshot->second)
    {
        SetStorageError(error, "credential file changed after it was loaded");
        return false;
    }
    return true;
}

bool R18SourceCredentialStore::LoadUidLocked(int uid, std::string* error)
{
    by_uid_.erase(uid);
    loaded_envelopes_.erase(uid);

    std::unordered_map<std::string, R18SourceCredential> table;
    const auto path = PathForUid(uid);
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory || (!ec && !std::filesystem::exists(status)))
    {
        by_uid_[uid] = std::move(table);
        loaded_envelopes_[uid] = std::nullopt;
        return true;
    }
    if (ec || std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status) ||
        !TightenCredentialFilePermissions(path))
    {
        SetStorageError(error, "credential file is not a secure regular file");
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        SetStorageError(error, "credential file cannot be opened");
        return false;
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    JsonValue root;
    if (!json::glaze_parse(root, content) || !root.is_object() ||
        json::glaze_safe_get<int>(root, "version", 0) != kCredentialEnvelopeVersion ||
        json::glaze_safe_get<std::string>(root, "algorithm", "") != kCredentialEnvelopeAlgorithm)
    {
        SetStorageError(error, "credential file is not an encrypted v1 envelope");
        return false;
    }

    std::string plaintext;
    if (!DecryptCredentialPayload(uid, root, plaintext, error))
        return false;

    JsonValue payload;
    const bool payload_valid = json::glaze_parse(payload, plaintext) && payload.is_array();
    SecureClear(plaintext);
    if (!payload_valid)
    {
        SetStorageError(error, "decrypted credential payload is invalid");
        return false;
    }

    for (std::size_t i = 0; i < payload.size(); ++i)
    {
        const auto item = payload[static_cast<int>(i)];
        R18SourceCredential cred;
        cred.source_id = json::glaze_safe_get<std::string>(item, "source_id", "");
        if (cred.source_id.empty())
            continue;
        cred.username = json::glaze_safe_get<std::string>(item, "username", "");
        cred.password = json::glaze_safe_get<std::string>(item, "password", "");
        cred.session_token = json::glaze_safe_get<std::string>(item, "session_token", "");
        cred.session_cookie = json::glaze_safe_get<std::string>(item, "session_cookie", "");
        cred.status = json::glaze_safe_get<std::string>(item, "status", "configured");
        cred.message = json::glaze_safe_get<std::string>(item, "message", "");
        cred.updated_at_ms = json::glaze_safe_get<int64_t>(item, "updated_at_ms", 0);
        table[cred.source_id] = std::move(cred);
    }
    by_uid_[uid] = std::move(table);
    loaded_envelopes_[uid] = content;
    return true;
}

bool R18SourceCredentialStore::SaveUidLocked(int uid, std::string* error)
{
    if (!EnsureRootLocked(error))
        return false;
    if (by_uid_.find(uid) == by_uid_.end() && !LoadUidLocked(uid, error))
        return false;
    if (!BackingFileMatchesLoadedSnapshotLocked(uid, error))
        return false;
    JsonValue arr{json::array_t{}};
    for (const auto& [id, cred] : by_uid_[uid])
    {
        JsonValue item;
        item["source_id"] = cred.source_id;
        item["username"] = cred.username;
        item["password"] = cred.password;
        item["session_token"] = cred.session_token;
        item["session_cookie"] = cred.session_cookie;
        item["status"] = cred.status;
        item["message"] = cred.message;
        item["updated_at_ms"] = cred.updated_at_ms;
        json::glaze_append(arr, item);
    }
    std::string plaintext = json::glaze_stringify(arr);
    JsonValue envelope;
    if (!EncryptCredentialPayload(uid, plaintext, envelope, error))
    {
        SecureClear(plaintext);
        return false;
    }
    SecureClear(plaintext);
    const std::string serialized_envelope = json::glaze_stringify(envelope);

    const auto path = PathForUid(uid);
    auto temporary_path = path;
    temporary_path += ".tmp";

    std::error_code ec;
    std::filesystem::remove(temporary_path, ec);
    if (ec)
    {
        SetStorageError(error, "stale credential temporary file cannot be removed");
        return false;
    }

    std::ofstream out(temporary_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open() || !TightenCredentialFilePermissions(temporary_path))
    {
        out.close();
        std::filesystem::remove(temporary_path, ec);
        SetStorageError(error, "credential temporary file cannot be secured");
        return false;
    }
    out << serialized_envelope;
    out.flush();
    if (!out.good())
    {
        out.close();
        std::filesystem::remove(temporary_path, ec);
        SetStorageError(error, "credential file cannot be written");
        return false;
    }
    out.close();

#if defined(_WIN32)
    if (!MoveFileExW(temporary_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(temporary_path, ec);
        SetStorageError(error, "credential file cannot be committed atomically");
        return false;
    }
#else
    ec.clear();
    std::filesystem::rename(temporary_path, path, ec);
    if (ec)
    {
        std::filesystem::remove(temporary_path, ec);
        SetStorageError(error, "credential file cannot be committed atomically");
        return false;
    }
#endif
    loaded_envelopes_[uid] = serialized_envelope;
    return true;
}

bool R18SourceCredentialStore::SaveUidOrReloadLocked(int uid, std::string* error)
{
    if (SaveUidLocked(uid, error))
        return true;
    LoadUidLocked(uid, nullptr);
    return false;
}

JsonValue R18SourceCredentialStore::ToPublicJson(const R18SourceCredential& cred) const
{
    JsonValue item;
    item["source_id"] = cred.source_id;
    item["username"] = cred.username;
    item["has_password"] = !cred.password.empty();
    item["has_session"] = !cred.session_token.empty() || !cred.session_cookie.empty();
    item["status"] = cred.status;
    item["message"] = cred.message;
    item["updated_at_ms"] = cred.updated_at_ms;
    // Never expose password/session secrets in API responses.
    return item;
}

JsonValue R18SourceCredentialStore::ListPublicAccounts(int uid)
{
    std::lock_guard<std::mutex> lock(mu_);
    JsonValue arr{json::array_t{}};
    if (!LoadUidLocked(uid, nullptr))
        return arr;
    for (const auto& [id, cred] : by_uid_.at(uid))
        json::glaze_append(arr, ToPublicJson(cred));
    return arr;
}

std::optional<R18SourceCredential> R18SourceCredentialStore::Get(int uid, const std::string& source_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, nullptr))
        return std::nullopt;
    const auto& table = by_uid_.at(uid);
    const auto it = table.find(source_id);
    if (it == table.end())
        return std::nullopt;
    return it->second;
}

bool R18SourceCredentialStore::UpsertLogin(int uid,
                                           const std::string& source_id,
                                           const std::string& username,
                                           const std::string& password,
                                           std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    cred.username = Trim(username);
    if (!password.empty())
        cred.password = password;
    if (cred.password.empty() && cred.username.empty() && cred.session_token.empty() && cred.session_cookie.empty())
        cred.status = "not_configured";
    else if (cred.session_token.empty() && cred.session_cookie.empty())
        cred.status = "configured";
    cred.message.clear();
    cred.updated_at_ms = NowMs();
    return SaveUidOrReloadLocked(uid, error);
}

bool R18SourceCredentialStore::Clear(int uid, const std::string& source_id, std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    by_uid_[uid].erase(sid);
    return SaveUidOrReloadLocked(uid, error);
}

bool R18SourceCredentialStore::UpdateSession(int uid,
                                             const std::string& source_id,
                                             const std::string& session_token,
                                             const std::string& session_cookie,
                                             const std::string& status,
                                             const std::string& message,
                                             std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    if (!session_token.empty())
        cred.session_token = session_token;
    if (!session_cookie.empty())
        cred.session_cookie = session_cookie;
    cred.status = status.empty() ? "authenticated" : status;
    cred.message = message;
    cred.updated_at_ms = NowMs();
    return SaveUidOrReloadLocked(uid, error);
}

bool R18SourceCredentialStore::ImportEhentaiSession(int uid,
                                                    const std::string& source_id,
                                                    const std::string& session_cookie,
                                                    const std::string& status,
                                                    const std::string& message,
                                                    std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }

    // Only allow E-Hentai family sources
    if (!IsEhentaiFamily(sid))
    {
        if (error)
            *error = "not_ehentai_source";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    // Clear username/password, store only session cookie
    cred.username.clear();
    cred.password.clear();
    cred.session_token.clear();
    cred.session_cookie = session_cookie;
    cred.status = status.empty() ? "authenticated" : status;
    cred.message = message;
    cred.updated_at_ms = NowMs();
    return SaveUidOrReloadLocked(uid, error);
}

bool R18SourceCredentialStore::ImportCookieSession(int uid,
                                                   const std::string& source_id,
                                                   const std::string& session_cookie,
                                                   const std::string& status,
                                                   const std::string& message,
                                                   std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    cred.session_token.clear();
    cred.session_cookie = session_cookie;
    cred.status = status.empty() ? "authenticated" : status;
    cred.message = message;
    cred.updated_at_ms = NowMs();
    return SaveUidOrReloadLocked(uid, error);
}

bool R18SourceCredentialStore::MarkError(int uid,
                                         const std::string& source_id,
                                         const std::string& message,
                                         std::string* error)
{
    // Preserve username/password on login failure so the user can retry without re-entering.
    // UpdateSession only mutates session/status/message fields and does not clear existing secrets.
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    if (!LoadUidLocked(uid, error))
        return false;
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    // Keep username/password/session as-is; only flip status for UI.
    cred.status = "error";
    cred.message = message;
    cred.updated_at_ms = NowMs();
    return SaveUidOrReloadLocked(uid, error);
}

} // namespace memochat::r18
