#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MomentItemInfo
{
    int seq = 0;
    std::string media_type; // text, image, video
    std::string media_key;
    std::string thumb_key;
    std::string content;
    int width = 0;
    int height = 0;
    int duration_ms = 0;
};

struct MomentContentInfo
{
    int64_t moment_id = 0;
    std::vector<MomentItemInfo> items;
    int64_t server_time = 0;
};

struct MomentInfo
{
    int64_t moment_id = 0;
    int uid = 0;
    int visibility = 0; // 0=public, 1=friends, 2=private
    std::string location;
    int64_t created_at = 0;
    int64_t deleted_at = 0;
    int like_count = 0;
    int comment_count = 0;
};

struct MomentLikeInfo
{
    int64_t id = 0;
    int64_t moment_id = 0;
    int uid = 0;
    std::string user_nick;
    std::string user_icon;
    int64_t created_at = 0;
};

struct MomentCommentInfo
{
    int64_t id = 0;
    int64_t moment_id = 0;
    int uid = 0;
    std::string user_nick;
    std::string user_icon;
    std::string content;
    int reply_uid = 0;
    std::string reply_nick;
    int64_t created_at = 0;
    int64_t deleted_at = 0;
    int like_count = 0;
    bool has_liked = false;
    std::vector<MomentLikeInfo> likes;
};
