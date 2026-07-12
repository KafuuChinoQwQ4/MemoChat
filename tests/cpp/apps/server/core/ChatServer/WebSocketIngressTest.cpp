#include <gtest/gtest.h>

#include "ChatFrameCodec.hpp"
#include "IChatSession.hpp"
#include "WebSocketChatServer.hpp"
#include "runtime/ExplicitThread.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

namespace beast = boost::beast;
namespace net = boost::asio;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

using memochat::chatserver::WebSocketChatServer;
using memochat::chatserver::transport::ChatFrame;
using memochat::chatserver::transport::ChatFrameCodec;
using memochat::chatserver::transport::ChatFrameDecodeStatus;

std::string ReserveLoopbackPort()
{
    net::io_context io_context;
    boost::system::error_code error;
    const auto address = net::ip::make_address("127.0.0.1", error);
    tcp::acceptor acceptor(io_context);
    if (!error)
    {
        acceptor.open(tcp::v4(), error);
    }
    if (!error)
    {
        acceptor.bind(tcp::endpoint(address, 0), error);
    }
    if (!error)
    {
        acceptor.listen(boost::asio::socket_base::max_listen_connections, error);
    }
    const auto endpoint = error ? tcp::endpoint{} : acceptor.local_endpoint(error);
    if (error)
    {
        ADD_FAILURE() << error.message();
        return "0";
    }
    return std::to_string(endpoint.port());
}

bool WaitUntil(std::function<bool()> predicate, std::chrono::milliseconds timeout = std::chrono::seconds(2))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

websocket::stream<tcp::socket>
ConnectClient(net::io_context& client_context, const std::string& port, const std::string& path = "/ws")
{
    tcp::resolver resolver(client_context);
    websocket::stream<tcp::socket> client(client_context);
    boost::system::error_code error;
    const auto endpoints = resolver.resolve("127.0.0.1", port, error);
    if (!error)
    {
        net::connect(client.next_layer(), endpoints, error);
    }
    if (!error)
    {
        client.handshake("127.0.0.1:" + port, path, error);
    }
    if (error)
    {
        ADD_FAILURE() << error.message();
    }
    return client;
}

class RunningWebSocketServer
{
public:
    explicit RunningWebSocketServer(WebSocketChatServer::InboundFrameCallback inbound_callback = {})
        : port(ReserveLoopbackPort())
        , server(std::make_shared<WebSocketChatServer>(server_context, std::move(inbound_callback)))
    {
        std::string error;
        EXPECT_TRUE(server->Start("127.0.0.1", port, "/ws", &error)) << error;
        EXPECT_TRUE(thread.Start(
            [this]() noexcept
            {
                server_context.run();
            },
            &error))
            << error;
    }

    ~RunningWebSocketServer()
    {
        if (server)
        {
            server->Stop();
        }
        server_context.stop();
        if (thread.Joinable())
        {
            std::string error;
            EXPECT_TRUE(thread.Join(&error)) << error;
        }
    }

    net::io_context server_context;
    std::string port;
    std::shared_ptr<WebSocketChatServer> server;
    memochat::runtime::ExplicitThread thread;
};

TEST(WebSocketIngressTest, BinaryFrameUsesSharedChatCodecAndCanWriteResponse)
{
    std::mutex mutex;
    std::condition_variable cv;
    bool received = false;
    short received_msg_id = 0;
    std::string received_payload;

    RunningWebSocketServer running(
        [&](const std::shared_ptr<IChatSession>& session, short msg_id, std::string_view payload)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                received = true;
                received_msg_id = msg_id;
                received_payload = std::string(payload);
            }
            cv.notify_one();
            session->send(R"({"ok":true})", 7002);
            return true;
        });

    net::io_context client_context;
    auto client = ConnectClient(client_context, running.port);

    const auto encoded = ChatFrameCodec::Encode(7001, R"({"hello":"websocket"})");
    ASSERT_TRUE(encoded.has_value());
    client.binary(true);
    boost::system::error_code client_error;
    client.write(net::buffer(*encoded), client_error);
    ASSERT_FALSE(client_error) << client_error.message();

    {
        std::unique_lock<std::mutex> lock(mutex);
        ASSERT_TRUE(cv.wait_for(lock,
                                std::chrono::seconds(2),
                                [&]
                                {
                                    return received;
                                }));
        EXPECT_EQ(received_msg_id, 7001);
        EXPECT_EQ(received_payload, R"({"hello":"websocket"})");
    }

    beast::flat_buffer response_buffer;
    client.read(response_buffer, client_error);
    ASSERT_FALSE(client_error) << client_error.message();
    const auto response = beast::buffers_to_string(response_buffer.data());
    std::vector<std::uint8_t> response_bytes(response.begin(), response.end());

    ChatFrame decoded;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(response_bytes, decoded), ChatFrameDecodeStatus::Complete);
    EXPECT_TRUE(response_bytes.empty());
    EXPECT_EQ(decoded.msg_id, 7002);
    EXPECT_EQ(decoded.payload, R"({"ok":true})");

    client.close(websocket::close_code::normal, client_error);
    ASSERT_FALSE(client_error) << client_error.message();
    EXPECT_TRUE(WaitUntil(
        [&]
        {
            return running.server->sessionCount() == 0U;
        }));
}

TEST(WebSocketIngressTest, TextFramesAreRejected)
{
    RunningWebSocketServer running;

    net::io_context client_context;
    auto client = ConnectClient(client_context, running.port);

    client.text(true);
    boost::system::error_code write_error;
    client.write(net::buffer(std::string("not-a-chat-frame")), write_error);
    ASSERT_FALSE(write_error) << write_error.message();

    beast::flat_buffer buffer;
    beast::error_code ec;
    client.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
}

TEST(WebSocketIngressTest, StopClosesActiveSessions)
{
    RunningWebSocketServer running;

    net::io_context client_context;
    auto client = ConnectClient(client_context, running.port);
    ASSERT_TRUE(WaitUntil(
        [&]
        {
            return running.server->sessionCount() == 1U;
        }));

    running.server->Stop();
    EXPECT_FALSE(running.server->isRunning());
    EXPECT_EQ(running.server->sessionCount(), 0U);

    beast::flat_buffer buffer;
    beast::error_code ec;
    client.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
}

} // namespace
