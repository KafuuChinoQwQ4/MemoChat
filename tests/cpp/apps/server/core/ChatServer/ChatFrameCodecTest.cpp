#include <gtest/gtest.h>

#include "ChatFrameCodec.hpp"

#include "const.hpp"
#include "MsgNode.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using memochat::chatserver::transport::ChatFrame;
using memochat::chatserver::transport::ChatFrameCodec;
using memochat::chatserver::transport::ChatFrameDecodeStatus;

TEST(ChatFrameCodecTest, EncodesAndDecodesOneFixedHeaderFrame)
{
    const auto encoded = ChatFrameCodec::Encode(1005, R"({"uid":7})");
    ASSERT_TRUE(encoded.has_value());
    ASSERT_EQ(encoded->size(), static_cast<std::size_t>(HEAD_TOTAL_LEN + 9));

    auto buffer = *encoded;
    ChatFrame frame;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(buffer, frame), ChatFrameDecodeStatus::Complete);
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(frame.msg_id, 1005);
    EXPECT_EQ(frame.payload, R"({"uid":7})");
}

TEST(ChatFrameCodecTest, KeepsPartialFrameBuffered)
{
    const auto encoded = ChatFrameCodec::Encode(1023, "{}");
    ASSERT_TRUE(encoded.has_value());

    std::vector<std::uint8_t> buffer(encoded->begin(), encoded->begin() + HEAD_TOTAL_LEN + 1);
    ChatFrame frame;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(buffer, frame), ChatFrameDecodeStatus::NeedMoreData);
    EXPECT_EQ(buffer.size(), static_cast<std::size_t>(HEAD_TOTAL_LEN + 1));

    buffer.insert(buffer.end(), encoded->begin() + HEAD_TOTAL_LEN + 1, encoded->end());
    EXPECT_EQ(ChatFrameCodec::DecodeOne(buffer, frame), ChatFrameDecodeStatus::Complete);
    EXPECT_EQ(frame.msg_id, 1023);
    EXPECT_EQ(frame.payload, "{}");
}

TEST(ChatFrameCodecTest, DecodesCoalescedFramesInOrder)
{
    const auto first = ChatFrameCodec::Encode(1001, "one");
    const auto second = ChatFrameCodec::Encode(1002, "two");
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());

    auto buffer = *first;
    buffer.insert(buffer.end(), second->begin(), second->end());

    ChatFrame frame;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(buffer, frame), ChatFrameDecodeStatus::Complete);
    EXPECT_EQ(frame.msg_id, 1001);
    EXPECT_EQ(frame.payload, "one");
    EXPECT_EQ(ChatFrameCodec::DecodeOne(buffer, frame), ChatFrameDecodeStatus::Complete);
    EXPECT_EQ(frame.msg_id, 1002);
    EXPECT_EQ(frame.payload, "two");
    EXPECT_TRUE(buffer.empty());
}

TEST(ChatFrameCodecTest, RejectsOversizedPayload)
{
    const std::string too_large(static_cast<std::size_t>(MAX_LENGTH + 1), 'x');
    EXPECT_FALSE(ChatFrameCodec::Encode(1001, too_large).has_value());
}

TEST(ChatFrameCodecTest, LegacyVarintRejectsOverlongUint32Encoding)
{
    const std::uint8_t raw[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x01};
    const std::uint8_t* in = raw;
    const std::uint8_t* end = raw + sizeof(raw);

    EXPECT_EQ(VarintCodec::decode(in, end), std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(in, end);
}

TEST(ChatFrameCodecTest, LegacyFrameEncodedSizeAllowsMaxVarintHeaders)
{
    EXPECT_EQ(FrameCodec::encodedSize(static_cast<std::size_t>(MAX_LENGTH), std::numeric_limits<std::uint32_t>::max()),
              static_cast<std::size_t>(1 + 3 + 5 + MAX_LENGTH));
}
