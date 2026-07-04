#pragma once

#include "routing/RouteSchema.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace memochat::gate::routing
{

struct RouteSchemaModuleEntry
{
    std::string module;
    std::vector<RouteSchemaDescriptor> routes;
};

class RouteSchemaCatalog
{
public:
    void Add(std::string module, std::vector<RouteSchemaDescriptor> routes)
    {
        modules_.push_back(RouteSchemaModuleEntry{std::move(module), std::move(routes)});
    }

    const std::vector<RouteSchemaModuleEntry>& Modules() const
    {
        return modules_;
    }

    std::vector<RouteSchemaDescriptor> AllRoutes() const
    {
        std::vector<RouteSchemaDescriptor> all;
        for (const auto& entry : modules_)
        {
            for (const auto& route : entry.routes)
            {
                all.push_back(route);
            }
        }
        return all;
    }

private:
    std::vector<RouteSchemaModuleEntry> modules_;
};

inline std::vector<std::string> FindDuplicateRoutePaths(const RouteSchemaCatalog& catalog)
{
    const std::vector<RouteSchemaDescriptor> all = catalog.AllRoutes();

    std::unordered_map<std::string, int> counts;
    counts.reserve(all.size());
    for (const auto& route : all)
    {
        const std::string key = route.method + " " + route.path;
        ++counts[key];
    }

    std::vector<std::string> duplicates;
    std::unordered_map<std::string, bool> emitted;
    for (const auto& route : all)
    {
        const std::string key = route.method + " " + route.path;
        if (counts[key] >= 2 && !emitted[key])
        {
            duplicates.push_back(key);
            emitted[key] = true;
        }
    }
    return duplicates;
}

inline std::string RenderRouteSchemaCatalogSnapshot(const RouteSchemaCatalog& catalog)
{
    std::string out;
    for (const auto& entry : catalog.Modules())
    {
        out += "== module: ";
        out += entry.module;
        out += " ==\n";
        out += RenderRouteSchemaSnapshot(entry.routes);
    }
    return out;
}

namespace detail
{

inline void AppendJsonEscaped(std::string& out, std::string_view s)
{
    static const char* kHexDigits = "0123456789abcdef";
    for (const char c : s)
    {
        const auto byte = static_cast<unsigned char>(c);
        switch (c)
        {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            default:
                if (byte < 0x20)
                {
                    out += "\\u00";
                    out += kHexDigits[(byte >> 4) & 0x0F];
                    out += kHexDigits[byte & 0x0F];
                }
                else
                {
                    out += c;
                }
                break;
        }
    }
}

inline void AppendBodyJson(std::string& out, const RouteBodySchema& body)
{
    out += "{\"type\":\"";
    AppendJsonEscaped(out, body.type_name);
    out += "\",\"fields\":[";
    for (std::size_t i = 0; i < body.fields.size(); ++i)
    {
        if (i != 0)
        {
            out += ",";
        }
        out += "\"";
        AppendJsonEscaped(out, body.fields[i].name);
        out += "\"";
    }
    out += "]}";
}

} // namespace detail

inline std::string RenderRouteSchemaCatalogJson(const RouteSchemaCatalog& catalog)
{
    std::string out;
    out += "{\"modules\":[";
    const auto& modules = catalog.Modules();
    for (std::size_t m = 0; m < modules.size(); ++m)
    {
        if (m != 0)
        {
            out += ",";
        }
        const auto& entry = modules[m];
        out += "{\"module\":\"";
        detail::AppendJsonEscaped(out, entry.module);
        out += "\",\"routes\":[";
        for (std::size_t r = 0; r < entry.routes.size(); ++r)
        {
            if (r != 0)
            {
                out += ",";
            }
            const auto& route = entry.routes[r];
            out += "{\"name\":\"";
            detail::AppendJsonEscaped(out, route.name);
            out += "\",\"method\":\"";
            detail::AppendJsonEscaped(out, route.method);
            out += "\",\"path\":\"";
            detail::AppendJsonEscaped(out, route.path);
            out += "\",\"request\":";
            detail::AppendBodyJson(out, route.request);
            out += ",\"response\":";
            detail::AppendBodyJson(out, route.response);
            out += "}";
        }
        out += "]}";
    }
    out += "]}";
    return out;
}

} // namespace memochat::gate::routing
