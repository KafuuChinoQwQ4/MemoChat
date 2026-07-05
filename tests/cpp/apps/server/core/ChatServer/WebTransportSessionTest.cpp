#include <gtest/gtest.h>

#include "ChatFrameCodec.hpp"
#include "IChatSession.hpp"
#include "WebTransportSession.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

using memochat::chatserver::WebTransportSession;
using memochat::chatserver::transport::ChatFrame;
using memochat::chatserver::transport::ChatFrameCodec;
using memochat::chatserver::transport::ChatFrameDecodeStatus;

std::string BytesToString(const std::vector<std::uint8_t>& bytes)
{
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

TEST(WebTransportSessionTest, SendEncodesChatFrameForProvider)
{
    std::vector<std::uint8_t> sent;
    auto session = std::make_shared<WebTransportSession>(
        [&](std::vector<std::uint8_t> frame)
        {
            sent = std::move(frame);
            return true;
        },
        [](const std::string&) {});

    session->Send(R"({"hello":"wt"})", 1005);

    ASSERT_FALSE(sent.empty());
    ChatFrame decoded;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(sent, decoded), ChatFrameDecodeStatus::Complete);
    EXPECT_TRUE(sent.empty());
    EXPECT_EQ(decoded.msg_id, 1005);
    EXPECT_EQ(decoded.payload, R"({"hello":"wt"})");
    EXPECT_EQ(session->transportName(), "webtransport");
}

TEST(WebTransportSessionTest, AcceptsPartialAndCoalescedStreamBytes)
{
    std::vector<std::pair<short, std::string>> received;
    auto session = std::make_shared<WebTransportSession>(
        [](std::vector<std::uint8_t>)
        {
            return true;
        },
        [](const std::string&) {},
        [&](const std::shared_ptr<IChatSession>&, short msg_id, std::string_view payload)
        {
            received.emplace_back(msg_id, std::string(payload));
            return true;
        });

    const auto first = ChatFrameCodec::Encode(1001, "one");
    const auto second = ChatFrameCodec::Encode(1002, "two");
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());

    const std::string first_bytes = BytesToString(*first);
    const std::string second_bytes = BytesToString(*second);

    EXPECT_TRUE(session->AcceptStreamBytes(std::string_view(first_bytes.data(), 3)));
    EXPECT_TRUE(received.empty());

    std::string rest_and_next = first_bytes.substr(3);
    rest_and_next.append(second_bytes);
    EXPECT_TRUE(session->AcceptStreamBytes(rest_and_next));

    ASSERT_EQ(received.size(), 2U);
    EXPECT_EQ(received[0].first, 1001);
    EXPECT_EQ(received[0].second, "one");
    EXPECT_EQ(received[1].first, 1002);
    EXPECT_EQ(received[1].second, "two");
    EXPECT_FALSE(session->isClosed());
}

TEST(WebTransportSessionTest, InvalidFrameClosesSession)
{
    int close_count = 0;
    int inbound_count = 0;
    auto session = std::make_shared<WebTransportSession>(
        [](std::vector<std::uint8_t>)
        {
            return true;
        },
        [&](const std::string&)
        {
            ++close_count;
        },
        [&](const std::shared_ptr<IChatSession>&, short, std::string_view)
        {
            ++inbound_count;
            return true;
        });

    const std::string invalid_negative_msg_id{"\xff\xff\x00\x00", 4};

    EXPECT_FALSE(session->AcceptStreamBytes(invalid_negative_msg_id));
    EXPECT_TRUE(session->isClosed());
    EXPECT_EQ(close_count, 1);
    EXPECT_EQ(inbound_count, 0);

    session->Close();
    EXPECT_EQ(close_count, 1);
}

TEST(WebTransportSessionTest, OversizedDecodeBufferClosesSession)
{
    int close_count = 0;
    int inbound_count = 0;
    auto session = std::make_shared<WebTransportSession>(
        [](std::vector<std::uint8_t>)
        {
            return true;
        },
        [&](const std::string&)
        {
            ++close_count;
        },
        [&](const std::shared_ptr<IChatSession>&, short, std::string_view)
        {
            ++inbound_count;
            return true;
        });

    const std::string oversized(WebTransportSession::kMaxDecodeBufferBytes + 1U, '\x80');

    EXPECT_FALSE(session->AcceptStreamBytes(oversized));
    EXPECT_TRUE(session->isClosed());
    EXPECT_EQ(close_count, 1);
    EXPECT_EQ(inbound_count, 0);
}

TEST(WebTransportSessionTest, ProviderSendFailureClosesSession)
{
    int close_count = 0;
    auto session = std::make_shared<WebTransportSession>(
        [](std::vector<std::uint8_t>)
        {
            return false;
        },
        [&](const std::string&)
        {
            ++close_count;
        });

    session->Send("{}", 1023);

    EXPECT_TRUE(session->isClosed());
    EXPECT_EQ(close_count, 1);
}

} // namespace
