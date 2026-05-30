#include "ChatFrameCodec.h"

#include <gtest/gtest.h>

#include <QByteArray>

namespace
{
unsigned char byteAt(const QByteArray& bytes, int index)
{
    return static_cast<unsigned char>(bytes.at(index));
}
} // namespace

TEST(ChatFrameCodecTest, EncodesBigEndianHeader)
{
    const QByteArray payload("abc", 3);
    const QByteArray frame = encodeChatFrame(ID_TEXT_CHAT_MSG_REQ, payload);
    const auto reqId = static_cast<quint16>(ID_TEXT_CHAT_MSG_REQ);

    ASSERT_EQ(frame.size(), 7);
    EXPECT_EQ(byteAt(frame, 0), static_cast<unsigned char>((reqId >> 8) & 0xff));
    EXPECT_EQ(byteAt(frame, 1), static_cast<unsigned char>(reqId & 0xff));
    EXPECT_EQ(byteAt(frame, 2), 0);
    EXPECT_EQ(byteAt(frame, 3), 3);
    EXPECT_EQ(frame.mid(4), payload);
}

TEST(ChatFrameCodecTest, BuffersPartialFrame)
{
    ChatFrameParser parser;
    const QByteArray payload("{\"ok\":true}", 11);
    const QByteArray frame = encodeChatFrame(ID_CHAT_LOGIN_RSP, payload);

    EXPECT_TRUE(parser.append(frame.left(3)).isEmpty());

    const QVector<ChatFrame> frames = parser.append(frame.mid(3));
    ASSERT_EQ(frames.size(), 1);
    EXPECT_EQ(frames[0].reqId, ID_CHAT_LOGIN_RSP);
    EXPECT_EQ(frames[0].length, payload.size());
    EXPECT_EQ(frames[0].payload, payload);
}

TEST(ChatFrameCodecTest, ParsesCoalescedFrames)
{
    ChatFrameParser parser;
    const QByteArray firstPayload("one", 3);
    const QByteArray secondPayload;
    const QByteArray bytes =
        encodeChatFrame(ID_TEXT_CHAT_MSG_REQ, firstPayload) + encodeChatFrame(ID_HEART_BEAT_REQ, secondPayload);

    const QVector<ChatFrame> frames = parser.append(bytes);
    ASSERT_EQ(frames.size(), 2);
    EXPECT_EQ(frames[0].reqId, ID_TEXT_CHAT_MSG_REQ);
    EXPECT_EQ(frames[0].length, firstPayload.size());
    EXPECT_EQ(frames[0].payload, firstPayload);
    EXPECT_EQ(frames[1].reqId, ID_HEART_BEAT_REQ);
    EXPECT_EQ(frames[1].length, 0);
    EXPECT_TRUE(frames[1].payload.isEmpty());
}

TEST(ChatFrameCodecTest, ResetDropsPartialState)
{
    ChatFrameParser parser;
    const QByteArray firstFrame = encodeChatFrame(ID_TEXT_CHAT_MSG_REQ, QByteArray("lost", 4));
    const QByteArray secondPayload("ok", 2);
    const QByteArray secondFrame = encodeChatFrame(ID_HEARTBEAT_RSP, secondPayload);

    EXPECT_TRUE(parser.append(firstFrame.left(2)).isEmpty());
    parser.reset();

    const QVector<ChatFrame> frames = parser.append(secondFrame);
    ASSERT_EQ(frames.size(), 1);
    EXPECT_EQ(frames[0].reqId, ID_HEARTBEAT_RSP);
    EXPECT_EQ(frames[0].length, secondPayload.size());
    EXPECT_EQ(frames[0].payload, secondPayload);
}
