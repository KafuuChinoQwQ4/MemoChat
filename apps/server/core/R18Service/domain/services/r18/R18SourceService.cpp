#include "r18/R18SourceService.hpp"

#include "r18/R18AdapterUtils.hpp"
#include "r18/R18JmAdapter.hpp"
#include "r18/R18PicacgAdapter.hpp"
#include "r18/R18SourceRecordCodec.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

import memochat.r18.source_algorithms;
import memochat.r18.source_service_algorithms;

namespace memochat::r18
{
namespace
{

using json::JsonValue;

const char* const kMockSourceId = source_service::modules::MockSourceId();

bool IsBuiltinSourceId(const std::string& id)
{
    return id == kMockSourceId || id == kJmSourceId || id == kPicacgSourceId;
}

JsonValue ErrorData(const std::string& source_id, const std::string& message)
{
    JsonValue data;
    data["source_id"] = source_id;
    data["items"] = JsonValue{json::array_t{}};
    data["error_message"] = message;

    JsonValue item;
    item["source_id"] = source_id;
    item["comic_id"] = "";
    item["title"] = "官方源请求失败";
    item["subtitle"] = message;
    item["description"] = message;
    item["cover"] = "";
    item["tags"] = JsonValue{json::array_t{}};
    json::glaze_append(data["items"], item);
    return data;
}

std::filesystem::path ResolveDataRoot()
{
    const auto cwd = std::filesystem::current_path();
    std::string leaf = cwd.filename().string();
    modules::LowerAsciiInPlace(leaf.data(), leaf.size());
    const auto base = leaf.rfind(source_service::modules::GateShellPrefix(), 0) == 0 ? cwd.parent_path() : cwd;
    return base / "data" / "r18" / "sources";
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix)
{
    if (value.size() < suffix.size())
        return false;
    return std::equal(suffix.rbegin(),
                      suffix.rend(),
                      value.rbegin(),
                      [](char a, char b)
                      {
                          return modules::EqualsAsciiCaseInsensitive(a, b);
                      });
}

std::string Stem(const std::string& file_name)
{
    std::filesystem::path p(file_name);
    auto stem = p.stem().string();
    return stem.empty() ? "imported-source" : stem;
}

std::string NormalizeId(std::string value)
{
    for (char& c : value)
    {
        c = modules::NormalizeSourceIdChar(static_cast<unsigned char>(c));
    }
    value.erase(std::unique(value.begin(),
                            value.end(),
                            [](char a, char b)
                            {
                                return a == '-' && b == '-';
                            }),
                value.end());
    while (!value.empty() && value.front() == '-')
        value.erase(value.begin());
    while (!value.empty() && value.back() == '-')
        value.pop_back();
    return value.empty() ? "imported-source" : value;
}

std::string TrimAscii(std::string value)
{
    const auto is_space = [](unsigned char c)
    {
        return modules::IsAsciiSpace(c);
    };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string NormalizePathSegment(std::string value, const std::string& fallback)
{
    for (char& c : value)
    {
        c = modules::NormalizePathSegmentChar(static_cast<unsigned char>(c));
    }
    value.erase(std::unique(value.begin(),
                            value.end(),
                            [](char a, char b)
                            {
                                return a == '-' && b == '-';
                            }),
                value.end());
    while (!value.empty() && modules::IsPathSegmentTrimChar(value.front()))
        value.erase(value.begin());
    while (!value.empty() && modules::IsPathSegmentTrimChar(value.back()))
        value.pop_back();
    return value.empty() ? fallback : value;
}

bool LooksLikeJavaScript(const std::string& file_name, const std::string& manifest_json, const std::string& binary)
{
    if (EndsWithCaseInsensitive(file_name, ".js"))
        return true;
    if (!manifest_json.empty())
    {
        JsonValue manifest;
        if (json::glaze_parse(manifest, manifest_json))
        {
            const std::string format = json::glaze_safe_get<std::string>(manifest, "format", "");
            if (format == source_service::modules::SourceJsFormat() || format == "javascript")
                return true;
        }
    }
    const auto probe =
        binary.substr(0, std::min<std::size_t>(binary.size(), source_service::modules::ManifestProbeWindow()));
    return source_service::modules::MatchesJsSourceProbe(probe.find("class ") != std::string::npos,
                                                         probe.find("ComicSource") != std::string::npos,
                                                         probe.find("search") != std::string::npos);
}

} // namespace

R18SourceService& R18SourceService::Instance()
{
    static R18SourceService svc;
    return svc;
}

R18SourceService::R18SourceService()
{
    data_root_ = ResolveDataRoot();
    image_cache_root_ = data_root_.parent_path() / "image-cache";
    std::error_code ec;
    std::filesystem::create_directories(data_root_, ec);
    std::filesystem::create_directories(image_cache_root_, ec);
    InstallBuiltinSourcesLocked();
    LoadLocked();
}

void R18SourceService::InstallBuiltinSourcesLocked()
{
    const auto install = [this](const char* id, const char* name, const char* version)
    {
        if (sources_.find(id) != sources_.end())
            return;
        R18SourceRecord rec;
        rec.id = id;
        rec.name = name;
        rec.version = version;
        rec.format = source_service::modules::NativeFormat();
        rec.enabled = true;
        rec.builtin = true;
        rec.status = source_service::modules::OkStatus();
        sources_[rec.id] = rec;
    };
    install(kJmSourceId, "禁漫天堂", source_service::modules::JmSourceVersion());
    install(kPicacgSourceId, "哔咔漫画", source_service::modules::PicacgSourceVersion());
}

json::JsonValue R18SourceService::ListSources()
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    json::JsonValue arr{json::array_t{}};
    const auto append_source = [this, &arr](const std::string& id)
    {
        const auto it = sources_.find(id);
        if (it == sources_.end())
            return;
        json::JsonValue rec = R18SourceRecordToJsonValue(it->second);
        if (!json::glaze_has_key(rec, "title") || json::glaze_safe_get<std::string>(rec, "title", "").empty())
            rec["title"] = it->second.name;
        json::glaze_append(arr, rec);
    };
    append_source(kJmSourceId);
    append_source(kPicacgSourceId);
    append_source(kMockSourceId);
    for (const auto& [id, source] : sources_)
    {
        if (id != kJmSourceId && id != kPicacgSourceId && id != kMockSourceId)
        {
            json::JsonValue rec = R18SourceRecordToJsonValue(source);
            if (!json::glaze_has_key(rec, "title") || json::glaze_safe_get<std::string>(rec, "title", "").empty())
                rec["title"] = source.name;
            json::glaze_append(arr, rec);
        }
    }
    return arr;
}

bool R18SourceService::EnableSource(const std::string& id, bool enabled, std::string* error)
{
    const std::string normalized_id = TrimAscii(id);
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(normalized_id);
    if (it == sources_.end())
    {
        if (error)
            *error = source_service::modules::SourceNotFoundMessage();
        return false;
    }
    it->second.enabled = enabled;
    SaveLocked();
    return true;
}

bool R18SourceService::DeleteSource(const std::string& id, std::string* error)
{
    const std::string normalized_id = TrimAscii(id);
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    if (IsBuiltinSourceId(normalized_id))
    {
        if (error)
            *error = source_service::modules::CannotDeleteBuiltinMessage();
        return false;
    }
    auto it = sources_.find(normalized_id);
    if (it == sources_.end())
    {
        if (error)
            *error = source_service::modules::SourceNotFoundMessage();
        return false;
    }
    // Remove files from disk
    const std::string path = it->second.path;
    if (!path.empty())
    {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(path).parent_path(), ec);
    }
    sources_.erase(it);
    SaveLocked();
    return true;
}

R18SourceRecord R18SourceService::ImportZip(const std::string& file_name,
                                            const std::string& manifest_json,
                                            const std::string& binary,
                                            std::string* error)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    const bool zip_payload = source_service::modules::IsZipMagic(binary.empty() ? '\0' : binary[0],
                                                                 binary.size() >= 2 ? binary[1] : '\0',
                                                                 binary.size());
    const bool js_payload = LooksLikeJavaScript(file_name, manifest_json, binary);
    if (!zip_payload && !js_payload)
    {
        if (error)
            *error = source_service::modules::InvalidPackagePayloadMessage();
        return {};
    }

    json::JsonValue manifest;
    if (!manifest_json.empty() && !json::glaze_parse(manifest, manifest_json))
    {
        if (error)
            *error = source_service::modules::InvalidManifestMessage();
        return {};
    }

    const std::string fallback_id = NormalizeId(Stem(file_name));
    R18SourceRecord rec;
    rec.id = manifest_json.empty() ? fallback_id : json::glaze_safe_get<std::string>(manifest, "id", fallback_id);
    rec.id = NormalizeId(rec.id);
    rec.name = manifest_json.empty() ? rec.id : json::glaze_safe_get<std::string>(manifest, "name", rec.id);
    rec.version =
        manifest_json.empty()
            ? source_service::modules::DefaultVersion()
            : json::glaze_safe_get<std::string>(manifest, "version", source_service::modules::DefaultVersion());
    rec.version = NormalizePathSegment(rec.version, source_service::modules::DefaultVersion());
    rec.format =
        js_payload ? source_service::modules::SourceJsFormat()
                   : json::glaze_safe_get<std::string>(manifest, "format", source_service::modules::NativeZipFormat());
    rec.source_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "source_url", "");
    rec.catalog_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "catalog_url", "");
    rec.enabled = false;
    rec.builtin = false;
    rec.status = js_payload ? source_service::modules::StagedJsStatus() : source_service::modules::StagedStatus();
    rec.message = js_payload ? "JavaScript source saved. Execution requires a MemoChat source runtime adapter."
                             : "Package staged. Build/unpack validation is handled by the plugin host deployment step.";

    if (rec.id.empty())
    {
        if (error)
            *error = source_service::modules::SourceIdEmptyMessage();
        return {};
    }
    if (IsBuiltinSourceId(rec.id))
    {
        if (error)
            *error = source_service::modules::ReservedSourceIdMessage();
        return {};
    }

    const auto dir = data_root_ / rec.id / rec.version;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        if (error)
            *error = source_service::modules::CreateDirFailedMessage();
        return {};
    }
    const auto source_path = dir / (js_payload ? "source.js" : "source.zip");
    std::ofstream out(source_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        if (error)
            *error = source_service::modules::PersistFailedMessage();
        return {};
    }
    out.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    out.close();
    if (!manifest_json.empty())
    {
        std::ofstream manifest_out(dir / "manifest.json", std::ios::binary | std::ios::trunc);
        manifest_out << manifest_json;
    }

    rec.path = source_path.string();
    sources_[rec.id] = rec;
    SaveLocked();
    return rec;
}

json::JsonValue R18SourceService::Search(const std::string& source_id, const std::string& keyword, int page)
{
    if (source_id == kJmSourceId)
    {
        try
        {
            return JmSearch(keyword, source_service::modules::NormalizeSearchPage(page));
        }
        catch (const std::exception& exc)
        {
            return ErrorData(kJmSourceId, exc.what());
        }
    }
    if (source_id == kPicacgSourceId)
    {
        try
        {
            return PicacgSearch(keyword, source_service::modules::NormalizeSearchPage(page));
        }
        catch (const std::exception& exc)
        {
            return ErrorData(kPicacgSourceId, exc.what());
        }
    }

    json::JsonValue data;
    data["source_id"] = source_id;
    data["keyword"] = keyword;
    data["page"] = page;
    data["items"] = json::JsonValue{json::array_t{}};
    data["max_page"] = 1;

    json::JsonValue first;
    first["source_id"] = source_id;
    first["comic_id"] = "mock-" + std::to_string(page) + "-1";
    first["title"] = keyword.empty() ? "R18 Preview Comic" : ("R18 Preview: " + keyword);
    first["subtitle"] = SourceSnapshot(source_id).value_or(R18SourceRecord{}).message;
    first["cover"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) + "&image_id=cover";
    first["author"] = source_id;
    first["tags"] = detail::MakeTags({"sample"});
    json::glaze_append(data["items"], first);
    return data;
}

json::JsonValue R18SourceService::Detail(const std::string& source_id, const std::string& comic_id)
{
    if (source_id == kJmSourceId)
    {
        try
        {
            return JmDetail(comic_id);
        }
        catch (const std::exception& exc)
        {
            json::JsonValue data;
            data["source_id"] = kJmSourceId;
            data["comic_id"] = comic_id;
            data["title"] = "官方源请求失败";
            data["description"] = exc.what();
            data["cover"] = "";
            data["chapters"] = json::JsonValue{json::array_t{}};
            return data;
        }
    }
    if (source_id == kPicacgSourceId)
    {
        try
        {
            return PicacgDetail(comic_id);
        }
        catch (const std::exception& exc)
        {
            json::JsonValue data;
            data["source_id"] = kPicacgSourceId;
            data["comic_id"] = comic_id;
            data["title"] = "官方源请求失败";
            data["description"] = exc.what();
            data["cover"] = "";
            data["chapters"] = json::JsonValue{json::array_t{}};
            return data;
        }
    }

    const auto source = SourceSnapshot(source_id);
    json::JsonValue data;
    data["source_id"] = source_id;
    data["comic_id"] = comic_id;
    data["title"] = (source && !source->name.empty() ? source->name : "Unknown Source") + " Preview Comic";
    data["description"] = source ? source->message : "The selected source was not found.";
    data["cover"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) + "&image_id=cover";
    data["chapters"] = json::JsonValue{json::array_t{}};
    for (int i = 1; i <= source_service::modules::PreviewChapterCount(); ++i)
    {
        json::JsonValue ch;
        ch["source_id"] = source_id;
        ch["comic_id"] = comic_id;
        ch["chapter_id"] = comic_id + "-ch" + std::to_string(i);
        ch["title"] = "Chapter " + std::to_string(i);
        ch["order"] = i;
        json::glaze_append(data["chapters"], ch);
    }
    return data;
}

json::JsonValue R18SourceService::Pages(const std::string& source_id, const std::string& chapter_id)
{
    if (source_id == kJmSourceId)
    {
        try
        {
            return JmPages(chapter_id);
        }
        catch (const std::exception& exc)
        {
            json::JsonValue data;
            data["source_id"] = kJmSourceId;
            data["chapter_id"] = chapter_id;
            data["error_message"] = exc.what();
            data["pages"] = json::JsonValue{json::array_t{}};
            return data;
        }
    }
    if (source_id == kPicacgSourceId)
    {
        const auto sep = chapter_id.find(':');
        const std::string comic_id = sep != std::string::npos ? chapter_id.substr(0, sep) : chapter_id;
        try
        {
            return PicacgPages(comic_id, chapter_id);
        }
        catch (const std::exception& exc)
        {
            json::JsonValue data;
            data["source_id"] = kPicacgSourceId;
            data["chapter_id"] = chapter_id;
            data["error_message"] = exc.what();
            data["pages"] = json::JsonValue{json::array_t{}};
            return data;
        }
    }

    json::JsonValue data;
    data["source_id"] = source_id;
    data["chapter_id"] = chapter_id;
    data["pages"] = json::JsonValue{json::array_t{}};
    for (int i = 1; i <= source_service::modules::PreviewPageCount(); ++i)
    {
        json::JsonValue page;
        page["index"] = i;
        page["image_id"] = chapter_id + "-p" + std::to_string(i);
        page["url"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) +
                      "&image_id=" + detail::UrlEncode(chapter_id + "-p" + std::to_string(i));
        json::glaze_append(data["pages"], page);
    }
    return data;
}

R18ImagePayload R18SourceService::FetchImage(const std::string& source_id, const std::string& image_url)
{
    if (source_id == kJmSourceId && !image_url.empty())
        return JmFetchImage(image_cache_root_, image_url);
    if (source_id == kPicacgSourceId && !image_url.empty())
        return PicacgFetchImage(image_cache_root_, image_url);
    return detail::PlaceholderImage("R18 Source Image", "preview");
}

std::optional<R18SourceRecord> R18SourceService::SourceSnapshot(const std::string& source_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(source_id);
    if (it == sources_.end())
        return std::nullopt;
    return it->second;
}

void R18SourceService::LoadLocked()
{
    const auto manifest_path = data_root_ / "sources.json";
    const bool has_shared_manifest = std::filesystem::exists(manifest_path);
    LoadManifestLocked(manifest_path);
    const auto legacy_root = std::filesystem::current_path() / "data" / "r18" / "sources";
    if (!has_shared_manifest && legacy_root != data_root_)
    {
        const auto before = sources_.size();
        LoadManifestLocked(legacy_root / "sources.json");
        if (sources_.size() != before)
            SaveLocked();
    }
}

void R18SourceService::LoadManifestLocked(const std::filesystem::path& manifest_path)
{
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in.is_open())
        return;
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    json::JsonValue root;
    if (!json::glaze_parse(root, content))
        return;
    json::JsonValue sources = json::glaze_get(root, "sources");
    auto arr = json::glaze_get_array(sources);
    if (!arr)
        return;
    for (const auto& item_json : *arr)
    {
        R18SourceRecord rec;
        if (!R18SourceRecordFromJsonValue(json::JsonValue(item_json), &rec))
            continue;
        if (!rec.id.empty() && !rec.builtin && !IsBuiltinSourceId(rec.id))
            sources_[rec.id] = rec;
    }
}

void R18SourceService::SaveLocked()
{
    json::JsonValue root;
    root["sources"] = json::JsonValue{json::array_t{}};
    for (const auto& [_, source] : sources_)
        if (!source.builtin)
            json::glaze_append(root["sources"], R18SourceRecordToJsonValue(source));
    std::ofstream out(data_root_ / "sources.json", std::ios::binary | std::ios::trunc);
    out << json::glaze_stringify(root);
}

} // namespace memochat::r18
