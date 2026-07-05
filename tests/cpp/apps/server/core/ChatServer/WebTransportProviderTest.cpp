#include <gtest/gtest.h>

#include "ChatFrameCodec.hpp"
#include "IWebTransportProvider.hpp"
#include "IWebTransportSession.hpp"
#include "WebTransportChatServer.hpp"
#include "WebTransportProviderFactory.hpp"

#include <boost/asio.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

using memochat::chatserver::IWebTransportProvider;
using memochat::chatserver::IWebTransportSession;
using memochat::chatserver::WebTransportChatServer;
using memochat::chatserver::WebTransportListenOptions;
using memochat::chatserver::WebTransportProviderSessionHooks;
using memochat::chatserver::transport::ChatFrame;
using memochat::chatserver::transport::ChatFrameCodec;
using memochat::chatserver::transport::ChatFrameDecodeStatus;

class FakeWebTransportProvider final : public IWebTransportProvider
{
public:
    bool Start(const WebTransportListenOptions& start_options,
               WebTransportProviderSessionHooks start_hooks,
               std::string* error) override
    {
        options = start_options;
        hooks = std::move(start_hooks);
        running = start_result;
        ++start_count;
        if (!start_result && error)
        {
            *error = start_error;
        }
        if (running && open_session_on_start && hooks.open_session)
        {
            opened_session = hooks.open_session(
                [this](std::vector<std::uint8_t> frame)
                {
                    sent_frames.push_back(std::move(frame));
                    return send_result;
                },
                [this](const std::string& session_id)
                {
                    provider_closed_sessions.push_back(session_id);
                });
        }
        return start_result;
    }

    void Stop() override
    {
        running = false;
        ++stop_count;
    }

    bool isRunning() const override
    {
        return running;
    }

    std::string providerName() const override
    {
        return "fake-webtransport";
    }

    bool start_result = true;
    bool open_session_on_start = true;
    bool send_result = true;
    bool running = false;
    int start_count = 0;
    int stop_count = 0;
    std::string start_error = "fake_start_failed";
    WebTransportListenOptions options;
    WebTransportProviderSessionHooks hooks;
    std::shared_ptr<IWebTransportSession> opened_session;
    std::vector<std::vector<std::uint8_t>> sent_frames;
    std::vector<std::string> provider_closed_sessions;
};

TEST(WebTransportProviderTest, StartDelegatesToProviderAndRegistersSession)
{
    boost::asio::io_context io_context;
    auto provider = std::make_unique<FakeWebTransportProvider>();
    auto* provider_ptr = provider.get();
    auto server = std::make_shared<WebTransportChatServer>(io_context, std::move(provider));

    std::string error = "stale_error";
    ASSERT_TRUE(server->Start("127.0.0.1", "9445", "/chat", &error));

    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(server->isRunning());
    EXPECT_EQ(server->providerName(), "fake-webtransport");
    EXPECT_EQ(server->listenHost(), "127.0.0.1");
    EXPECT_EQ(server->listenPort(), "9445");
    EXPECT_EQ(server->listenPath(), "/chat");
    EXPECT_EQ(provider_ptr->start_count, 1);
    EXPECT_EQ(provider_ptr->options.host, "127.0.0.1");
    EXPECT_EQ(provider_ptr->options.port, "9445");
    EXPECT_EQ(provider_ptr->options.path, "/chat");
    ASSERT_TRUE(provider_ptr->opened_session);
    EXPECT_EQ(provider_ptr->opened_session->transportName(), "webtransport");
    EXPECT_EQ(server->sessionCount(), 1U);
}

TEST(WebTransportProviderTest, RegisteredSessionSendsEncodedFramesThroughProvider)
{
    boost::asio::io_context io_context;
    auto provider = std::make_unique<FakeWebTransportProvider>();
    auto* provider_ptr = provider.get();
    auto server = std::make_shared<WebTransportChatServer>(io_context, std::move(provider));

    ASSERT_TRUE(server->Start("127.0.0.1", "9445", "/chat"));
    ASSERT_TRUE(provider_ptr->opened_session);

    provider_ptr->opened_session->send(R"({"hello":"provider"})", 409);

    ASSERT_EQ(provider_ptr->sent_frames.size(), 1U);
    ChatFrame decoded;
    EXPECT_EQ(ChatFrameCodec::DecodeOne(provider_ptr->sent_frames[0], decoded), ChatFrameDecodeStatus::Complete);
    EXPECT_TRUE(provider_ptr->sent_frames[0].empty());
    EXPECT_EQ(decoded.msg_id, 409);
    EXPECT_EQ(decoded.payload, R"({"hello":"provider"})");
}

TEST(WebTransportProviderTest, SessionCloseCleansServerRegistryAndProviderSession)
{
    boost::asio::io_context io_context;
    auto provider = std::make_unique<FakeWebTransportProvider>();
    auto* provider_ptr = provider.get();
    auto server = std::make_shared<WebTransportChatServer>(io_context, std::move(provider));

    ASSERT_TRUE(server->Start("127.0.0.1", "9445", "/chat"));
    ASSERT_TRUE(provider_ptr->opened_session);
    const auto session_id = provider_ptr->opened_session->sessionId();

    provider_ptr->opened_session->close();

    EXPECT_EQ(provider_ptr->provider_closed_sessions.size(), 1U);
    EXPECT_EQ(provider_ptr->provider_closed_sessions[0], session_id);
    EXPECT_EQ(server->sessionCount(), 0U);
}

TEST(WebTransportProviderTest, DefaultProviderReportsExpectedStartFailureWhenUnconfigured)
{
    boost::asio::io_context io_context;
    auto server = std::make_shared<WebTransportChatServer>(io_context);

    std::string error;
    EXPECT_FALSE(server->Start("127.0.0.1", "9445", "/chat", &error));

#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
    EXPECT_EQ(error, "webtransport_tls_certificate_required");
    EXPECT_EQ(server->providerName(), "libwebsockets");
#else
    EXPECT_EQ(error, "webtransport_h3_library_not_configured");
    EXPECT_EQ(server->providerName(), "unavailable");
#endif
    EXPECT_FALSE(server->isRunning());
    EXPECT_EQ(server->sessionCount(), 0U);
}

TEST(WebTransportProviderTest, DefaultFactoryKeepsProviderBehindBuildFeature)
{
    auto provider = memochat::chatserver::CreateDefaultWebTransportProvider();

    ASSERT_TRUE(provider);
#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
    EXPECT_EQ(provider->providerName(), "libwebsockets");
#else
    EXPECT_EQ(provider->providerName(), "unavailable");
#endif
}

} // namespace
