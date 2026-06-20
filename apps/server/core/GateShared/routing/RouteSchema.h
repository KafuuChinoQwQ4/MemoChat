#pragma once

#include "reflection/StdReflectionIntrospection.h"

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace memochat::gate::routing
{

struct RouteFieldSchema
{
    std::string name;
};

struct RouteFieldNameOverride
{
    std::string_view reflected_name;
    std::string_view schema_name;
};

struct RouteBodySchema
{
    std::string role;
    std::string type_name;
    std::vector<RouteFieldSchema> fields;
};

struct RouteSchemaDescriptor
{
    std::string method;
    std::string path;
    std::string name;
    RouteBodySchema request;
    RouteBodySchema response;
};

inline RouteBodySchema MakeEmptyBodySchema(std::string role, std::string type_name = {})
{
    RouteBodySchema schema;
    schema.role = std::move(role);
    schema.type_name = std::move(type_name);
    return schema;
}

inline std::string_view ResolveRouteFieldSchemaName(
    std::string_view reflected_name,
    std::initializer_list<RouteFieldNameOverride> field_name_overrides)
{
    for (const auto& override : field_name_overrides)
    {
        if (override.reflected_name == reflected_name)
        {
            return override.schema_name;
        }
    }
    return reflected_name;
}

template <typename T>
RouteBodySchema MakeBodySchema(std::string role,
                               std::string type_name,
                               std::initializer_list<RouteFieldNameOverride> field_name_overrides = {})
{
    RouteBodySchema schema;
    schema.role = std::move(role);
    schema.type_name = std::move(type_name);
#if MEMOCHAT_ENABLE_CPP26_REFLECTION
    constexpr auto field_names = memochat::reflection::FieldNames<T>();
    schema.fields.reserve(field_names.size());
    for (std::string_view field_name : field_names)
    {
        schema.fields.push_back(RouteFieldSchema{std::string(
            ResolveRouteFieldSchemaName(field_name, field_name_overrides))});
    }
#endif
    return schema;
}

inline RouteSchemaDescriptor MakeRouteSchemaWithoutBody(std::string method, std::string path, std::string name)
{
    RouteSchemaDescriptor descriptor;
    descriptor.method = std::move(method);
    descriptor.path = std::move(path);
    descriptor.name = std::move(name);
    descriptor.request = MakeEmptyBodySchema("request");
    descriptor.response = MakeEmptyBodySchema("response");
    return descriptor;
}

template <typename RequestDto, typename ResponseDto>
RouteSchemaDescriptor MakeRouteSchema(std::string method,
                                      std::string path,
                                      std::string name,
                                      std::string request_type_name,
                                      std::string response_type_name)
{
    RouteSchemaDescriptor descriptor;
    descriptor.method = std::move(method);
    descriptor.path = std::move(path);
    descriptor.name = std::move(name);
    descriptor.request = MakeBodySchema<RequestDto>("request", std::move(request_type_name));
    descriptor.response = MakeBodySchema<ResponseDto>("response", std::move(response_type_name));
    return descriptor;
}

template <typename RequestDto, typename ResponseDto>
RouteSchemaDescriptor MakeRouteSchema(
    std::string method,
    std::string path,
    std::string name,
    std::string request_type_name,
    std::string response_type_name,
    std::initializer_list<RouteFieldNameOverride> request_field_name_overrides,
    std::initializer_list<RouteFieldNameOverride> response_field_name_overrides)
{
    RouteSchemaDescriptor descriptor;
    descriptor.method = std::move(method);
    descriptor.path = std::move(path);
    descriptor.name = std::move(name);
    descriptor.request =
        MakeBodySchema<RequestDto>("request", std::move(request_type_name), request_field_name_overrides);
    descriptor.response =
        MakeBodySchema<ResponseDto>("response", std::move(response_type_name), response_field_name_overrides);
    return descriptor;
}

inline void AppendBodySchemaSnapshot(std::string& out, const RouteBodySchema& body)
{
    out += body.role;
    out += ": ";
    out += body.type_name.empty() ? "<none>" : body.type_name;
    out += "\n";
    for (const auto& field : body.fields)
    {
        out += "  - ";
        out += field.name;
        out += "\n";
    }
}

inline std::string RenderRouteSchemaSnapshot(const std::vector<RouteSchemaDescriptor>& routes)
{
    std::string out;
    for (const auto& route : routes)
    {
        out += "route: ";
        out += route.name;
        out += "\nmethod: ";
        out += route.method;
        out += "\npath: ";
        out += route.path;
        out += "\n";
        AppendBodySchemaSnapshot(out, route.request);
        AppendBodySchemaSnapshot(out, route.response);
        out += "\n";
    }
    return out;
}

} // namespace memochat::gate::routing
