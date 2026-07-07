#pragma once

#include "json/GlazeCompat.hpp"
#include "persistence/MomentTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace memochat::gate::services::moments
{

struct MomentPublishRequestDto
{
    int visibility = 0;
    std::string location;
    std::vector<MomentItemInfo> items;
};

struct MomentListRequestDto
{
    int64_t last_moment_id = 0;
    int limit = 20;
    int author_uid = 0;
};

struct MomentIdRequestDto
{
    int64_t moment_id = 0;
};

struct MomentLikeRequestDto
{
    int64_t moment_id = 0;
    bool like = true;
};

struct MomentCommentRequestDto
{
    int64_t moment_id = 0;
    std::string content;
    int reply_uid = 0;
    int64_t comment_id = 0;
    bool delete_ = false;
    bool delete_mode = false;
};

struct MomentCommentListRequestDto
{
    int64_t moment_id = 0;
    int64_t last_comment_id = 0;
    int limit = 20;
};

struct MomentCommentLikeRequestDto
{
    int64_t comment_id = 0;
    bool like = true;
};

int64_t MomentsReadInt64(const memochat::json::JsonValue& obj, const char* key, int64_t fallback = 0);
int MomentsReadInt(const memochat::json::JsonValue& obj, const char* key, int fallback = 0);
bool MomentsReadBool(const memochat::json::JsonValue& obj, const char* key, bool fallback = false);
std::string MomentsReadString(const memochat::json::JsonValue& obj, const char* key, const std::string& fallback = {});

void NormalizeMomentPublicItem(MomentItemInfo& item);

MomentPublishRequestDto MomentPublishRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentListRequestDto MomentListRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentIdRequestDto MomentIdRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentLikeRequestDto MomentLikeRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentCommentRequestDto MomentCommentRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentCommentListRequestDto MomentCommentListRequestFromJsonValue(const memochat::json::JsonValue& root);
MomentCommentLikeRequestDto MomentCommentLikeRequestFromJsonValue(const memochat::json::JsonValue& root);

} // namespace memochat::gate::services::moments
