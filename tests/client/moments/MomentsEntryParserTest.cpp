#include "MomentsEntryParser.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

QString gate_url_prefix;
QString gate_media_url_prefix = QStringLiteral("http://127.0.0.1:8080");

TEST(MomentsEntryParserTest, ParsesMomentEntryWithItemsLikesAndComments)
{
    QJsonObject item;
    item.insert(QStringLiteral("seq"), 1);
    item.insert(QStringLiteral("media_type"), QStringLiteral("image"));
    item.insert(QStringLiteral("media_key"), QStringLiteral("asset-1234567890123456"));
    item.insert(QStringLiteral("thumb_key"), QStringLiteral("thumb-1234567890123456"));
    item.insert(QStringLiteral("width"), 640);
    item.insert(QStringLiteral("height"), 480);

    QJsonObject like;
    like.insert(QStringLiteral("uid"), 12);
    like.insert(QStringLiteral("user_nick"), QString());
    like.insert(QStringLiteral("user_icon"), QStringLiteral("head_1.jpg"));
    like.insert(QStringLiteral("created_at"), static_cast<qint64>(1234567890000));

    QJsonObject commentLike;
    commentLike.insert(QStringLiteral("uid"), 13);
    commentLike.insert(QStringLiteral("user_nick"), QStringLiteral("Bob"));
    commentLike.insert(QStringLiteral("user_icon"), QStringLiteral("/res/head_2.png"));

    QJsonObject comment;
    comment.insert(QStringLiteral("id"), static_cast<qint64>(99));
    comment.insert(QStringLiteral("uid"), 14);
    comment.insert(QStringLiteral("user_nick"), QString());
    comment.insert(QStringLiteral("user_icon"), QStringLiteral("media/download?asset=avatar-key-1234567890"));
    comment.insert(QStringLiteral("content"), QStringLiteral("comment"));
    comment.insert(QStringLiteral("reply_uid"), 15);
    comment.insert(QStringLiteral("reply_nick"), QStringLiteral("ReplyNick"));
    comment.insert(QStringLiteral("created_at"), static_cast<qint64>(1234567891000));
    comment.insert(QStringLiteral("like_count"), -3);
    comment.insert(QStringLiteral("has_liked"), true);
    comment.insert(QStringLiteral("likes"), QJsonArray{commentLike});

    QJsonObject moment;
    moment.insert(QStringLiteral("moment_id"), static_cast<qint64>(88));
    moment.insert(QStringLiteral("uid"), 10);
    moment.insert(QStringLiteral("user_id"), QStringLiteral("u000000010"));
    moment.insert(QStringLiteral("user_name"), QStringLiteral("Alice"));
    moment.insert(QStringLiteral("user_nick"), QString());
    moment.insert(QStringLiteral("user_icon"), QStringLiteral("head_1.jpg"));
    moment.insert(QStringLiteral("visibility"), 1);
    moment.insert(QStringLiteral("location"), QStringLiteral("Shanghai"));
    moment.insert(QStringLiteral("created_at"), static_cast<qint64>(1234567890000));
    moment.insert(QStringLiteral("like_count"), 5);
    moment.insert(QStringLiteral("comment_count"), 1);
    moment.insert(QStringLiteral("has_liked"), true);
    moment.insert(QStringLiteral("items"), QJsonArray{item});
    moment.insert(QStringLiteral("likes"), QJsonArray{like});
    moment.insert(QStringLiteral("comments"), QJsonArray{comment});

    const MomentEntry entry = parseMomentEntryFromJson(moment);

    EXPECT_EQ(entry.momentId, static_cast<qint64>(88));
    EXPECT_EQ(entry.userNick, QStringLiteral("Alice"));
    EXPECT_EQ(entry.userIcon, QStringLiteral("qrc:/res/head_1.png"));
    ASSERT_EQ(entry.items.size(), 1);
    EXPECT_EQ(entry.items[0].mediaType, QStringLiteral("image"));
    EXPECT_EQ(entry.items[0].width, 640);
    ASSERT_EQ(entry.likes.size(), 1);
    EXPECT_EQ(entry.likes[0].userNick, QStringLiteral("用户"));
    EXPECT_EQ(entry.likes[0].userIcon, QStringLiteral("qrc:/res/head_1.png"));
    ASSERT_EQ(entry.comments.size(), 1);
    EXPECT_EQ(entry.comments[0].userNick, QStringLiteral("用户"));
    EXPECT_EQ(entry.comments[0].likeCount, 0);
    EXPECT_TRUE(entry.comments[0].hasLiked);
    ASSERT_EQ(entry.comments[0].likes.size(), 1);
    EXPECT_EQ(entry.comments[0].likes[0].userNick, QStringLiteral("Bob"));
    EXPECT_EQ(entry.comments[0].likes[0].userIcon, QStringLiteral("qrc:/res/head_2.png"));
}

TEST(MomentsEntryParserTest, ParsesCommentList)
{
    QJsonObject comment;
    comment.insert(QStringLiteral("id"), static_cast<qint64>(7));
    comment.insert(QStringLiteral("uid"), 8);
    comment.insert(QStringLiteral("user_nick"), QStringLiteral("Commenter"));
    comment.insert(QStringLiteral("content"), QStringLiteral("hello"));
    comment.insert(QStringLiteral("like_count"), 2);

    const QVector<MomentComment> comments = parseMomentComments(QJsonArray{comment});

    ASSERT_EQ(comments.size(), 1);
    EXPECT_EQ(comments[0].id, static_cast<qint64>(7));
    EXPECT_EQ(comments[0].uid, 8);
    EXPECT_EQ(comments[0].userNick, QStringLiteral("Commenter"));
    EXPECT_EQ(comments[0].content, QStringLiteral("hello"));
    EXPECT_EQ(comments[0].likeCount, 2);
}
