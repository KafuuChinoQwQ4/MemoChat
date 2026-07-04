#pragma once

#include <glaze/glaze.hpp>

#include <string>
#include <string_view>

namespace memochat::json
{

template <typename T> bool WriteTypedJson(const T& value, std::string* out, std::string* error_out = nullptr)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    out->clear();
    auto ec = glz::write_json(value, *out);
    if (ec)
    {
        if (error_out != nullptr)
        {
            *error_out = glz::format_error(ec, *out);
        }
        return false;
    }
    return true;
}

template <typename T> std::string WriteTypedJsonOrEmptyObject(const T& value)
{
    std::string out;
    return WriteTypedJson(value, &out) ? out : "{}";
}

template <typename T> bool ReadTypedJson(std::string_view body, T* out, std::string* error_out = nullptr)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    auto ec = glz::read_json(*out, body);
    if (ec)
    {
        if (error_out != nullptr)
        {
            *error_out = glz::format_error(ec, body);
        }
        return false;
    }
    return true;
}

} // namespace memochat::json
