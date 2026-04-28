#include "H1R18Routes.h"

#include "H1Connection.h"
#include "H1JsonSupport.h"
#include "H1LogicSystem.h"
#include "RedisMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;

namespace {

using memochat::json::JsonValue;

struct R18SourceRecord {
    std::string id;
    std::string name;
    std::string version;
    std::string path;
    bool enabled = true;
    bool builtin = false;
    std::string status = "ok";
    std::string message;
};

class R18SourceService {
public:
    static R18SourceService& Instance()
    {
        static R18SourceService svc;
        return svc;
    }

    JsonValue ListSources()
    {
        std::lock_guard<std::mutex> lock(mu_);
        JsonValue arr{memochat::json::array_t{}};
        for (const auto& [_, source] : sources_) {
            memochat::json::glaze_append(arr, ToJson(source));
        }
        return arr;
    }

    bool EnableSource(const std::string& id, bool enabled, std::string* error)
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = sources_.find(id);
        if (it == sources_.end()) {
            if (error) *error = "source not found";
            return false;
        }
        it->second.enabled = enabled;
        SaveLocked();
        return true;
    }

    R18SourceRecord ImportZip(const std::string& file_name,
                              const std::string& manifest_json,
                              const std::string& binary,
                              std::string* error)
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (binary.size() < 4 || binary[0] != 'P' || binary[1] != 'K') {
            if (error) *error = "plugin package must be a zip file";
            return {};
        }

        JsonValue manifest;
        if (!manifest_json.empty() && !memochat::json::glaze_parse(manifest, manifest_json)) {
            if (error) *error = "manifest_json is invalid";
            return {};
        }

        const std::string fallback_id = NormalizeId(Stem(file_name));
        R18SourceRecord rec;
        rec.id = manifest_json.empty() ? fallback_id : memochat::json::glaze_safe_get<std::string>(manifest, "id", fallback_id);
        rec.name = manifest_json.empty() ? rec.id : memochat::json::glaze_safe_get<std::string>(manifest, "name", rec.id);
        rec.version = manifest_json.empty() ? "0.0.0" : memochat::json::glaze_safe_get<std::string>(manifest, "version", "0.0.0");
        rec.enabled = false;
        rec.builtin = false;
        rec.status = "staged";
        rec.message = "Package staged. Build/unpack validation is handled by the plugin host deployment step.";

        if (rec.id.empty()) {
            if (error) *error = "source id is empty";
            return {};
        }

        const auto dir = data_root_ / rec.id / rec.version;
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            if (error) *error = "failed to create source directory";
            return {};
        }
        const auto zip_path = dir / "source.zip";
        std::ofstream out(zip_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            if (error) *error = "failed to persist plugin package";
            return {};
        }
        out.write(binary.data(), static_cast<std::streamsize>(binary.size()));
        out.close();
        if (!manifest_json.empty()) {
            std::ofstream manifest_out(dir / "manifest.json", std::ios::binary | std::ios::trunc);
            manifest_out << manifest_json;
        }

        rec.path = zip_path.string();
        sources_[rec.id] = rec;
        SaveLocked();
        return rec;
    }

    JsonValue Search(const std::string& source_id, const std::string& keyword, int page, int uid, const std::string& token)
    {
        JsonValue data;
        data["source_id"] = source_id;
        data["keyword"] = keyword;
        data["page"] = page;
        data["items"] = JsonValue{memochat::json::array_t{}};

        JsonValue first;
        first["source_id"] = source_id;
        first["comic_id"] = "mock-" + std::to_string(page) + "-1";
        first["title"] = keyword.empty() ? "R18 Mock Comic" : ("R18 Mock Comic: " + keyword);
        first["subtitle"] = "C++ source plugin placeholder";
        first["cover"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + token + "&source_id=" + source_id + "&image_id=cover";
        first["author"] = "Mock Source";
        first["tags"] = JsonValue{memochat::json::array_t{}};
        memochat::json::glaze_append(first["tags"], "sample");
        memochat::json::glaze_append(data["items"], first);
        return data;
    }

    JsonValue Detail(const std::string& source_id, const std::string& comic_id, int uid, const std::string& token)
    {
        JsonValue data;
        data["source_id"] = source_id;
        data["comic_id"] = comic_id;
        data["title"] = "R18 Mock Comic";
        data["description"] = "This is a low-coupling placeholder returned by the R18 source service. Real content comes from imported C++ source plugins.";
        data["cover"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + token + "&source_id=" + source_id + "&image_id=cover";
        data["chapters"] = JsonValue{memochat::json::array_t{}};
        for (int i = 1; i <= 3; ++i) {
            JsonValue ch;
            ch["source_id"] = source_id;
            ch["comic_id"] = comic_id;
            ch["chapter_id"] = comic_id + "-ch" + std::to_string(i);
            ch["title"] = "Chapter " + std::to_string(i);
            ch["order"] = i;
            memochat::json::glaze_append(data["chapters"], ch);
        }
        return data;
    }

    JsonValue Pages(const std::string& source_id, const std::string& chapter_id, int uid, const std::string& token)
    {
        JsonValue data;
        data["source_id"] = source_id;
        data["chapter_id"] = chapter_id;
        data["pages"] = JsonValue{memochat::json::array_t{}};
        for (int i = 1; i <= 5; ++i) {
            JsonValue page;
            page["index"] = i;
            page["image_id"] = chapter_id + "-p" + std::to_string(i);
            page["url"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + token + "&source_id=" + source_id + "&image_id=" + chapter_id + "-p" + std::to_string(i);
            memochat::json::glaze_append(data["pages"], page);
        }
        return data;
    }

private:
    R18SourceService()
    {
        data_root_ = std::filesystem::current_path() / "data" / "r18" / "sources";
        std::error_code ec;
        std::filesystem::create_directories(data_root_, ec);
        R18SourceRecord builtin;
        builtin.id = "mock";
        builtin.name = "Mock C++ Source";
        builtin.version = "1.0.0";
        builtin.enabled = true;
        builtin.builtin = true;
        builtin.status = "ok";
        sources_[builtin.id] = builtin;
    }

    static std::string Stem(const std::string& file_name)
    {
        std::filesystem::path p(file_name);
        auto stem = p.stem().string();
        return stem.empty() ? "imported-source" : stem;
    }

    static std::string NormalizeId(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            if (std::isalnum(c)) return static_cast<char>(std::tolower(c));
            return '-';
        });
        value.erase(std::unique(value.begin(), value.end(), [](char a, char b) { return a == '-' && b == '-'; }), value.end());
        while (!value.empty() && value.front() == '-') value.erase(value.begin());
        while (!value.empty() && value.back() == '-') value.pop_back();
        return value.empty() ? "imported-source" : value;
    }

    static JsonValue ToJson(const R18SourceRecord& source)
    {
        JsonValue item;
        item["id"] = source.id;
        item["name"] = source.name;
        item["version"] = source.version;
        item["enabled"] = source.enabled;
        item["builtin"] = source.builtin;
        item["status"] = source.status;
        item["message"] = source.message;
        return item;
    }

    void SaveLocked()
    {
        JsonValue root;
        root["sources"] = JsonValue{memochat::json::array_t{}};
        for (const auto& [_, source] : sources_) {
            if (!source.builtin) {
                memochat::json::glaze_append(root["sources"], ToJson(source));
            }
        }
        std::ofstream out(data_root_ / "sources.json", std::ios::binary | std::ios::trunc);
        out << memochat::json::glaze_stringify(root);
    }

    std::mutex mu_;
    std::filesystem::path data_root_;
    std::unordered_map<std::string, R18SourceRecord> sources_;
};

bool DecodeBase64(const std::string& input, std::string& out)
{
    static constexpr unsigned char kInvalid = 255;
    unsigned char table[256];
    std::fill(std::begin(table), std::end(table), kInvalid);
    for (int i = 0; i < 26; ++i) {
        table[static_cast<unsigned char>('A' + i)] = static_cast<unsigned char>(i);
        table[static_cast<unsigned char>('a' + i)] = static_cast<unsigned char>(26 + i);
    }
    for (int i = 0; i < 10; ++i) table[static_cast<unsigned char>('0' + i)] = static_cast<unsigned char>(52 + i);
    table[static_cast<unsigned char>('+')] = 62;
    table[static_cast<unsigned char>('/')] = 63;

    out.clear();
    int val = 0;
    int bits = -8;
    for (unsigned char c : input) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        if (table[c] == kInvalid) return false;
        val = (val << 6) + table[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return true;
}

bool ValidateUserToken(int uid, const std::string& token)
{
    if (uid <= 0 || token.empty()) return false;
    std::string token_value;
    return RedisMgr::GetInstance()->Get(std::string(USERTOKENPREFIX) + std::to_string(uid), token_value) && token_value == token;
}

bool RequireAuth(const JsonValue& src, JsonValue& root, int& uid, std::string& token)
{
    uid = memochat::json::glaze_safe_get<int64_t>(src, "uid", 0LL);
    token = memochat::json::glaze_safe_get<std::string>(src, "token", "");
    if (!ValidateUserToken(uid, token)) {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid";
        return false;
    }
    return true;
}

void WriteOk(JsonValue& root, const JsonValue& data)
{
    root["error"] = ErrorCodes::Success;
    root["message"] = "";
    root["data"] = data;
}

} // namespace

void H1R18ServiceRoutes::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegGet("/api/r18/sources", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        JsonValue root;
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        if (!ValidateUserToken(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid";
        } else {
            JsonValue data;
            data["sources"] = R18SourceService::Instance().ListSources();
            WriteOk(root, data);
        }
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });

    logic.RegPost("/api/r18/source/import", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string file_name = memochat::json::glaze_safe_get<std::string>(src, "file_name", "source.zip");
            const std::string encoded = memochat::json::glaze_safe_get<std::string>(src, "data_base64", "");
            const std::string manifest_json = memochat::json::glaze_safe_get<std::string>(src, "manifest_json", "");
            std::string binary;
            if (encoded.empty() || !DecodeBase64(encoded, binary)) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = "invalid plugin package payload";
                return true;
            }
            std::string error;
            auto rec = R18SourceService::Instance().ImportZip(file_name, manifest_json, binary, &error);
            if (!error.empty()) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }
            JsonValue source;
            source["id"] = rec.id;
            source["name"] = rec.name;
            source["version"] = rec.version;
            source["enabled"] = rec.enabled;
            source["status"] = rec.status;
            source["message"] = rec.message;
            JsonValue data;
            data["source"] = source;
            WriteOk(root, data);
            return true;
        });
    });

    logic.RegPost("/api/r18/source/enable", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            std::string error;
            if (!R18SourceService::Instance().EnableSource(source_id, true, &error)) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }
            JsonValue data;
            data["source_id"] = source_id;
            data["enabled"] = true;
            WriteOk(root, data);
            return true;
        });
    });

    logic.RegPost("/api/r18/source/disable", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            std::string error;
            if (!R18SourceService::Instance().EnableSource(source_id, false, &error)) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }
            JsonValue data;
            data["source_id"] = source_id;
            data["enabled"] = false;
            WriteOk(root, data);
            return true;
        });
    });

    logic.RegPost("/api/r18/search", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "mock");
            const std::string keyword = memochat::json::glaze_safe_get<std::string>(src, "keyword", "");
            const int page = static_cast<int>(memochat::json::glaze_safe_get<int64_t>(src, "page", 1LL));
            WriteOk(root, R18SourceService::Instance().Search(source_id, keyword, page, uid, token));
            return true;
        });
    });

    logic.RegPost("/api/r18/comic/detail", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "mock");
            const std::string comic_id = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
            WriteOk(root, R18SourceService::Instance().Detail(source_id, comic_id, uid, token));
            return true;
        });
    });

    logic.RegPost("/api/r18/chapter/pages", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "mock");
            const std::string chapter_id = memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
            WriteOk(root, R18SourceService::Instance().Pages(source_id, chapter_id, uid, token));
            return true;
        });
    });

    logic.RegPost("/api/r18/favorite/toggle", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            JsonValue data;
            data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
            data["favorited"] = memochat::json::glaze_safe_get<bool>(src, "favorited", true);
            WriteOk(root, data);
            return true;
        });
    });

    logic.RegPost("/api/r18/history/update", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection, [](const JsonValue& src, JsonValue& root, const std::string&) {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token)) return true;
            JsonValue data;
            data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
            data["chapter_id"] = memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
            data["page_index"] = memochat::json::glaze_safe_get<int64_t>(src, "page_index", 0LL);
            WriteOk(root, data);
            return true;
        });
    });

    logic.RegGet("/api/r18/history", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        JsonValue root;
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        if (!ValidateUserToken(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid";
        } else {
            JsonValue data;
            data["items"] = JsonValue{memochat::json::array_t{}};
            WriteOk(root, data);
        }
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });

    logic.RegGet("/api/r18/image", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "image/svg+xml");
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        if (!ValidateUserToken(uid, token)) {
            connection->_response.set(http::field::content_type, "text/plain");
            beast::ostream(connection->_response.body()) << "token invalid";
            return true;
        }
        const std::string image_id = connection->_get_params["image_id"];
        std::ostringstream svg;
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"720\" height=\"1080\" viewBox=\"0 0 720 1080\">"
            << "<rect width=\"720\" height=\"1080\" fill=\"#201923\"/>"
            << "<rect x=\"48\" y=\"48\" width=\"624\" height=\"984\" rx=\"18\" fill=\"#2f2734\" stroke=\"#f2a7c5\" stroke-width=\"3\"/>"
            << "<text x=\"360\" y=\"506\" fill=\"#f8dce7\" font-size=\"38\" text-anchor=\"middle\" font-family=\"Arial\">R18 Source Image</text>"
            << "<text x=\"360\" y=\"562\" fill=\"#d6bac6\" font-size=\"22\" text-anchor=\"middle\" font-family=\"Arial\">" << image_id << "</text>"
            << "</svg>";
        beast::ostream(connection->_response.body()) << svg.str();
        return true;
    });
}
