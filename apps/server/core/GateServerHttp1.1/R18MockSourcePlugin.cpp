#include "R18PluginProtocol.h"

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#define R18_EXPORT __declspec(dllexport)
#else
#define R18_EXPORT __attribute__((visibility("default")))
#endif

namespace {

const char* CopyJson(const std::string& json)
{
    auto* out = static_cast<char*>(std::malloc(json.size() + 1));
    if (!out) {
        return nullptr;
    }
    std::memcpy(out, json.c_str(), json.size() + 1);
    return out;
}

} // namespace

extern "C" R18_EXPORT int r18_plugin_abi_version()
{
    return MEMOCHAT_R18_PLUGIN_ABI_VERSION;
}

extern "C" R18_EXPORT const char* r18_plugin_call(const char* request_json)
{
    const std::string req = request_json ? request_json : "";
    if (req.find("\"method\":\"source.info\"") != std::string::npos) {
        return CopyJson(R"({"error":0,"data":{"id":"mock.native","name":"Native Mock Source","version":"1.0.0","adult":true}})");
    }
    if (req.find("\"method\":\"source.search\"") != std::string::npos) {
        return CopyJson(R"({"error":0,"data":{"items":[{"source_id":"mock.native","comic_id":"native-1","title":"Native Mock Comic","subtitle":"Returned by C ABI plugin","cover":""}]}})");
    }
    if (req.find("\"method\":\"comic.detail\"") != std::string::npos) {
        return CopyJson(R"({"error":0,"data":{"source_id":"mock.native","comic_id":"native-1","title":"Native Mock Comic","description":"Mock detail from native plugin","chapters":[{"source_id":"mock.native","comic_id":"native-1","chapter_id":"native-1-ch1","title":"Chapter 1","order":1}]}})");
    }
    if (req.find("\"method\":\"chapter.pages\"") != std::string::npos) {
        return CopyJson(R"({"error":0,"data":{"source_id":"mock.native","chapter_id":"native-1-ch1","pages":[{"index":1,"image_id":"native-1","url":""}]}})");
    }
    return CopyJson(R"({"error":404,"message":"unsupported method"})");
}

extern "C" R18_EXPORT void r18_plugin_free(const char* response_json)
{
    std::free(const_cast<char*>(response_json));
}

extern "C" R18_EXPORT void r18_plugin_shutdown()
{
}
