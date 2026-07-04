#pragma once

#include "r18/R18SourceService.hpp"

#include <string>
#include <string_view>

namespace memochat::r18
{

struct R18SourceRecordDto
{
    std::string id;
    std::string name;
    std::string version;
    std::string path;
    std::string format;
    std::string source_url;
    std::string catalog_url;
    bool enabled = false;
    bool builtin = false;
    std::string status;
    std::string message;
};

R18SourceRecordDto ToR18SourceRecordDto(const R18SourceRecord& record);
R18SourceRecord ToR18SourceRecord(const R18SourceRecordDto& dto);
bool EncodeR18SourceRecord(const R18SourceRecord& record, std::string* out, std::string* error_out = nullptr);
bool DecodeR18SourceRecord(std::string_view body, R18SourceRecord* out, std::string* error_out = nullptr);
memochat::json::JsonValue R18SourceRecordToJsonValue(const R18SourceRecord& record);
bool R18SourceRecordFromJsonValue(const memochat::json::JsonValue& value,
                                  R18SourceRecord* out,
                                  std::string* error_out = nullptr);

} // namespace memochat::r18
