#include "r18/R18SourceRecordCodec.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.r18.source_record_codec_algorithms;

namespace memochat::r18
{
namespace
{

std::string DefaultString(std::string value, const std::string& fallback)
{
    return source_record_codec::modules::ShouldUseFallbackString(value.empty()) ? fallback : value;
}

} // namespace

R18SourceRecordDto ToR18SourceRecordDto(const R18SourceRecord& record)
{
    R18SourceRecordDto dto;
    dto.id = record.id;
    dto.name = record.name;
    dto.version = record.version;
    dto.path = record.path;
    dto.format = record.format;
    dto.source_url = record.source_url;
    dto.catalog_url = record.catalog_url;
    dto.enabled = record.enabled;
    dto.builtin = record.builtin;
    dto.status = record.status;
    dto.message = record.message;
    return dto;
}

R18SourceRecord ToR18SourceRecord(const R18SourceRecordDto& dto)
{
    R18SourceRecord record;
    record.id = dto.id;
    record.name = DefaultString(dto.name, record.id);
    record.version = DefaultString(dto.version, "0.0.0");
    record.path = dto.path;
    record.format = DefaultString(dto.format, "native-zip");
    record.source_url = dto.source_url;
    record.catalog_url = dto.catalog_url;
    record.enabled = dto.enabled;
    record.builtin = dto.builtin;
    record.status = DefaultString(dto.status, "staged");
    record.message = dto.message;
    return record;
}

bool EncodeR18SourceRecord(const R18SourceRecord& record, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToR18SourceRecordDto(record), out, error_out);
}

bool DecodeR18SourceRecord(std::string_view body, R18SourceRecord* out, std::string* error_out)
{
    if (!source_record_codec::modules::HasDecodeOutput(out != nullptr))
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    R18SourceRecordDto dto;
    if (!memochat::json::ReadTypedJson(body, &dto, error_out))
    {
        return false;
    }
    *out = ToR18SourceRecord(dto);
    return source_record_codec::modules::HasRequiredSourceRecordIdentity(out->id.empty());
}

memochat::json::JsonValue R18SourceRecordToJsonValue(const R18SourceRecord& record)
{
    std::string body;
    if (!EncodeR18SourceRecord(record, &body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    memochat::json::JsonValue value;
    if (!memochat::json::glaze_parse(value, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return value;
}

memochat::json::JsonValue R18SourceRecordToPublicJsonValue(const R18SourceRecord& record)
{
    auto value = R18SourceRecordToJsonValue(record);
    if (value.isObject())
    {
        value.impl().get<memochat::json::object_t>().erase("path");
    }
    return value;
}

bool R18SourceRecordFromJsonValue(const memochat::json::JsonValue& value, R18SourceRecord* out, std::string* error_out)
{
    return DecodeR18SourceRecord(memochat::json::glaze_stringify(value), out, error_out);
}

} // namespace memochat::r18
