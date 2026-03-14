#pragma once

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <mutex>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace memochat::auth {

struct ChatLoginTicketClaims {
    int uid = 0;
    std::string user_id;
    std::string name;
    std::string nick;
    std::string icon;
    std::string desc;
    std::string email;
    int sex = 0;
    std::string target_server;
    int protocol_version = 3;
    int64_t issued_at_ms = 0;
    int64_t expire_at_ms = 0;
};

inline std::string Base64UrlEncode(const std::string& input) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    uint32_t val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) | c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    return out;
}

inline bool Base64UrlDecode(const std::string& input, std::string& output) {
    static const std::vector<int> table = []() {
        std::vector<int> t(256, -1);
        for (int i = 'A'; i <= 'Z'; ++i) t[i] = i - 'A';
        for (int i = 'a'; i <= 'z'; ++i) t[i] = i - 'a' + 26;
        for (int i = '0'; i <= '9'; ++i) t[i] = i - '0' + 52;
        t[static_cast<unsigned char>('-')] = 62;
        t[static_cast<unsigned char>('_')] = 63;
        return t;
    }();

    output.clear();
    uint32_t val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        const int decoded = table[c];
        if (decoded < 0) {
            return false;
        }
        val = (val << 6) | static_cast<uint32_t>(decoded);
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return true;
}

inline bool HmacSha256(const std::string& key, const std::string& data, std::string& output) {
#ifdef _WIN32
    output.clear();
    static BCRYPT_ALG_HANDLE alg = nullptr;
    static std::once_flag init_once;
    static bool init_ok = false;
    std::call_once(init_once, []() {
        init_ok = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) >= 0;
    });
    if (!init_ok || alg == nullptr) {
        return false;
    }

    BCRYPT_HASH_HANDLE hash = nullptr;
    PUCHAR hash_object = nullptr;
    DWORD object_size = 0;
    DWORD data_size = 0;
    DWORD hash_size = 0;
    NTSTATUS status = 0;
    auto cleanup = [&]() {
        if (hash != nullptr) {
            BCryptDestroyHash(hash);
        }
        if (hash_object != nullptr) {
            HeapFree(GetProcessHeap(), 0, hash_object);
        }
    };
    status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size),
                               sizeof(object_size), &data_size, 0);
    if (status < 0) {
        cleanup();
        return false;
    }
    status = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hash_size),
                               sizeof(hash_size), &data_size, 0);
    if (status < 0) {
        cleanup();
        return false;
    }
    hash_object = static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(), 0, object_size));
    if (hash_object == nullptr) {
        cleanup();
        return false;
    }
    status = BCryptCreateHash(alg, &hash, hash_object, object_size,
                              reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())),
                              static_cast<ULONG>(key.size()), 0);
    if (status < 0) {
        cleanup();
        return false;
    }
    status = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
                            static_cast<ULONG>(data.size()), 0);
    if (status < 0) {
        cleanup();
        return false;
    }
    output.resize(hash_size);
    status = BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(output.data()), hash_size, 0);
    cleanup();
    return status >= 0;
#else
    (void)key;
    (void)data;
    output.clear();
    return false;
#endif
}

inline Json::Value ClaimsToJson(const ChatLoginTicketClaims& claims) {
    Json::Value root(Json::objectValue);
    root["uid"] = claims.uid;
    root["user_id"] = claims.user_id;
    root["name"] = claims.name;
    root["nick"] = claims.nick;
    root["icon"] = claims.icon;
    root["desc"] = claims.desc;
    root["email"] = claims.email;
    root["sex"] = claims.sex;
    root["target_server"] = claims.target_server;
    root["protocol_version"] = claims.protocol_version;
    root["issued_at_ms"] = static_cast<Json::Int64>(claims.issued_at_ms);
    root["expire_at_ms"] = static_cast<Json::Int64>(claims.expire_at_ms);
    return root;
}

inline bool ClaimsFromJson(const Json::Value& root, ChatLoginTicketClaims& claims) {
    if (!root.isObject()) {
        return false;
    }
    claims.uid = root.get("uid", 0).asInt();
    claims.user_id = root.get("user_id", "").asString();
    claims.name = root.get("name", "").asString();
    claims.nick = root.get("nick", "").asString();
    claims.icon = root.get("icon", "").asString();
    claims.desc = root.get("desc", "").asString();
    claims.email = root.get("email", "").asString();
    claims.sex = root.get("sex", 0).asInt();
    claims.target_server = root.get("target_server", "").asString();
    claims.protocol_version = root.get("protocol_version", 0).asInt();
    claims.issued_at_ms = root.get("issued_at_ms", 0).asInt64();
    claims.expire_at_ms = root.get("expire_at_ms", 0).asInt64();
    return claims.uid > 0 && !claims.target_server.empty();
}

inline std::string EncodeTicket(const ChatLoginTicketClaims& claims, const std::string& secret) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::string payload = Json::writeString(builder, ClaimsToJson(claims));
    std::string mac;
    if (!HmacSha256(secret, payload, mac)) {
        return std::string();
    }
    return Base64UrlEncode(payload) + "." + Base64UrlEncode(mac);
}

inline bool DecodeAndVerifyTicket(const std::string& ticket,
                                  const std::string& secret,
                                  ChatLoginTicketClaims& claims,
                                  std::string* error_code = nullptr) {
    const auto sep = ticket.find('.');
    if (sep == std::string::npos) {
        if (error_code) {
            *error_code = "format";
        }
        return false;
    }
    std::string payload;
    std::string signature;
    if (!Base64UrlDecode(ticket.substr(0, sep), payload) ||
        !Base64UrlDecode(ticket.substr(sep + 1), signature)) {
        if (error_code) {
            *error_code = "base64";
        }
        return false;
    }
    std::string expected_mac;
    if (!HmacSha256(secret, payload, expected_mac) || expected_mac != signature) {
        if (error_code) {
            *error_code = "signature";
        }
        return false;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(payload, root) || !ClaimsFromJson(root, claims)) {
        if (error_code) {
            *error_code = "payload";
        }
        return false;
    }
    return true;
}

} // namespace memochat::auth
