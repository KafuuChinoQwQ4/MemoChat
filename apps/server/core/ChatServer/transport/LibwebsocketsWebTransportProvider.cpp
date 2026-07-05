#include "LibwebsocketsWebTransportProvider.hpp"

#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER

#include "logging/Logger.hpp"

#include <libwebsockets.h>
extern "C"
{
#include <libwebsockets/lws-webtransport.h>
}

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace memochat::chatserver
{

struct LibwebsocketsWebTransportProvider::SessionState
{
    std::string session_id;
    std::shared_ptr<IWebTransportSession> session;
    lws* session_wsi = nullptr;
    lws* stream_wsi = nullptr;
    std::deque<std::vector<std::uint8_t>> outbound;
    bool close_requested = false;
    bool closed = false;
};

namespace
{
constexpr int kServicePollMs = 50;
constexpr int kRxBufferSize = 64 * 1024;

const lws_protocols kProtocols[] = {
    {"webtransport", &LibwebsocketsWebTransportProvider::Callback, 0, kRxBufferSize, 0, nullptr, 0},
    LWS_PROTOCOL_LIST_TERM};

int ParsePort(const std::string& value)
{
    if (value.empty())
    {
        return 8445;
    }
    try
    {
        const auto parsed = std::stoi(value);
        if (parsed > 0 && parsed <= 65535)
        {
            return parsed;
        }
    }
    catch (...)
    {
    }
    return -1;
}

std::string HeaderValue(lws* wsi, enum lws_token_indexes token)
{
    const int length = lws_hdr_total_length(wsi, token);
    if (length <= 0)
    {
        return {};
    }

    std::vector<char> buffer(static_cast<std::size_t>(length) + 1U, '\0');
    const int copied = lws_hdr_copy(wsi, buffer.data(), static_cast<int>(buffer.size()), token);
    if (copied <= 0)
    {
        return {};
    }
    return std::string(buffer.data(), static_cast<std::size_t>(copied));
}

std::string_view StripQuery(std::string_view value)
{
    const auto query_pos = value.find('?');
    return query_pos == std::string_view::npos ? value : value.substr(0, query_pos);
}

bool ShouldBindToNamedInterface(std::string_view host)
{
    if (host.empty() || host == "0.0.0.0" || host == "127.0.0.1" || host == "::" || host == "::1" ||
        host == "localhost")
    {
        return false;
    }
    return host.find('.') == std::string_view::npos && host.find(':') == std::string_view::npos;
}

bool TraceEnabled()
{
    const char* value = std::getenv("MEMOCHAT_TRACE_WEBTRANSPORT_PROVIDER");
    return value != nullptr && value[0] != '\0' && std::string_view(value) != "0";
}

void TraceEvent(std::string_view event,
                const lws* wsi = nullptr,
                const lws* parent = nullptr,
                std::size_t length = 0,
                std::string_view session_id = {})
{
    if (!TraceEnabled())
    {
        return;
    }

    std::cerr << "[webtransport.provider.lws] event=" << event;
    if (wsi != nullptr)
    {
        std::cerr << " wsi=" << static_cast<const void*>(wsi);
    }
    if (parent != nullptr)
    {
        std::cerr << " parent=" << static_cast<const void*>(parent);
    }
    if (length != 0)
    {
        std::cerr << " len=" << length;
    }
    if (!session_id.empty())
    {
        std::cerr << " session_id=" << session_id;
    }
    std::cerr << '\n';
}
} // namespace

LibwebsocketsWebTransportProvider::LibwebsocketsWebTransportProvider() = default;

LibwebsocketsWebTransportProvider::~LibwebsocketsWebTransportProvider()
{
    Stop();
}

bool LibwebsocketsWebTransportProvider::Start(const WebTransportListenOptions& options,
                                              WebTransportProviderSessionHooks hooks,
                                              std::string* error)
{
    Stop();

    if (error)
    {
        error->clear();
    }
    if (!hooks.open_session)
    {
        if (error)
        {
            *error = "webtransport_open_session_hook_missing";
        }
        return false;
    }
    if (options.cert_file.empty() || options.private_key_file.empty())
    {
        if (error)
        {
            *error = "webtransport_tls_certificate_required";
        }
        return false;
    }

    const int port = ParsePort(options.port);
    if (port < 0)
    {
        if (error)
        {
            *error = "webtransport_invalid_port";
        }
        return false;
    }

    _options = options;
    if (_options.host.empty())
    {
        _options.host = "127.0.0.1";
    }
    if (_options.path.empty())
    {
        _options.path = "/chat";
    }
    _hooks = std::move(hooks);
    _stopping.store(false, std::memory_order_release);

    if (TraceEnabled())
    {
        lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, nullptr);
    }

    lws_context_creation_info info;
    std::memset(&info, 0, sizeof(info));
    info.port = port;
    info.iface = ShouldBindToNamedInterface(_options.host) ? _options.host.c_str() : nullptr;
    info.protocols = kProtocols;
    info.user = this;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
    info.ssl_cert_filepath = _options.cert_file.c_str();
    info.ssl_private_key_filepath = _options.private_key_file.c_str();
    info.alpn = "h3";

    _context = lws_create_context(&info);
    if (_context == nullptr)
    {
        if (error)
        {
            *error = "webtransport_lws_context_create_failed";
        }
        return false;
    }

    _running.store(true, std::memory_order_release);
    _service_thread = std::thread(&LibwebsocketsWebTransportProvider::serviceLoop, this);
    memolog::LogInfo("webtransport.provider.lws.start",
                     "libwebsockets WebTransport provider started",
                     {{"host", _options.host}, {"port", std::to_string(port)}, {"path", _options.path}});
    return true;
}

void LibwebsocketsWebTransportProvider::Stop()
{
    _stopping.store(true, std::memory_order_release);
    if (_context != nullptr)
    {
        lws_cancel_service(_context);
    }
    if (_service_thread.joinable())
    {
        _service_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions_by_id.clear();
        _states_by_wsi.clear();
    }

    if (_context != nullptr)
    {
        lws_context_destroy(_context);
        _context = nullptr;
    }
    _running.store(false, std::memory_order_release);
}

bool LibwebsocketsWebTransportProvider::isRunning() const
{
    return _running.load(std::memory_order_acquire) && !_stopping.load(std::memory_order_acquire);
}

std::string LibwebsocketsWebTransportProvider::providerName() const
{
    return "libwebsockets";
}

int LibwebsocketsWebTransportProvider::Callback(lws* wsi,
                                                enum lws_callback_reasons reason,
                                                void* user,
                                                void* in,
                                                std::size_t len)
{
    auto* context = lws_get_context(wsi);
    auto* provider = context ? static_cast<LibwebsocketsWebTransportProvider*>(lws_context_user(context)) : nullptr;
    if (provider == nullptr)
    {
        return 0;
    }
    return provider->handleCallback(wsi, reason, user, in, len);
}

int LibwebsocketsWebTransportProvider::handleCallback(lws* wsi,
                                                      enum lws_callback_reasons reason,
                                                      void* user,
                                                      void* in,
                                                      std::size_t len)
{
    (void) user;
    switch (reason)
    {
        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
        case LWS_CALLBACK_ESTABLISHED:
            return handleEstablishedWsi(wsi);

        case LWS_CALLBACK_RECEIVE:
        {
            if (lws_wt_is_session(wsi))
            {
                TraceEvent("session_receive_ignored", wsi, lws_get_parent(wsi), len);
                return 0;
            }
            std::shared_ptr<SessionState> state;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                state = stateForWsiLocked(wsi);
            }
            if (!state || !state->session || state->closed)
            {
                TraceEvent("receive_no_state", wsi, lws_get_parent(wsi), len);
                return -1;
            }
            TraceEvent("receive", wsi, lws_get_parent(wsi), len, state->session_id);
            const auto bytes = std::string_view(static_cast<const char*>(in), len);
            return state->session->acceptStreamBytes(bytes) ? 0 : -1;
        }

        case LWS_CALLBACK_SERVER_WRITEABLE:
        {
            std::shared_ptr<SessionState> state;
            std::vector<std::uint8_t> frame;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                state = stateForWsiLocked(wsi);
                if (!state || state->closed || state->outbound.empty())
                {
                    return 0;
                }
                frame = std::move(state->outbound.front());
                state->outbound.pop_front();
            }

            const int written =
                writeFrameToWsi(wsi, frame, state ? state->session_id : std::string(), "writeable", "write_ok");
            if (written != static_cast<int>(frame.size()))
            {
                memolog::LogWarn("webtransport.provider.lws.write_failed",
                                 "libwebsockets WebTransport stream write failed",
                                 {{"session_id", state ? state->session_id : std::string()}});
                if (state && state->session)
                {
                    state->session->close();
                }
                return -1;
            }
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (state && !state->outbound.empty() && state->stream_wsi == wsi)
                {
                    lws_callback_on_writable(wsi);
                }
            }
            return 0;
        }

        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        {
            std::lock_guard<std::mutex> lock(_mutex);
            TraceEvent("event_wait_cancelled");
            applyPendingClosesLocked();
            requestWritableForPendingLocked();
            return 0;
        }

        case LWS_CALLBACK_CLOSED:
        {
            std::shared_ptr<SessionState> state;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                state = stateForWsiLocked(wsi);
                if (state)
                {
                    closeStateLocked(state);
                }
                removeWsiLocked(wsi);
            }
            if (state && state->session)
            {
                state->session->close();
            }
            return 0;
        }

        case LWS_CALLBACK_CHILD_CLOSING:
            return 0;

        default:
            return 0;
    }
}

int LibwebsocketsWebTransportProvider::handleEstablishedWsi(lws* wsi)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (stateForWsiLocked(wsi))
        {
            return 0;
        }
    }

    if (isSessionWsi(wsi))
    {
        return openSession(wsi) ? 0 : -1;
    }

    bool has_parent_session = false;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        has_parent_session = stateForWsiLocked(lws_get_parent(wsi)) != nullptr;
    }
    if (has_parent_session)
    {
        return attachStream(wsi) ? 0 : -1;
    }

    return 0;
}

bool LibwebsocketsWebTransportProvider::isSessionWsi(lws* wsi) const
{
    if (lws_wt_is_session(wsi))
    {
        return true;
    }

    return HeaderValue(wsi, WSI_TOKEN_COLON_PROTOCOL) == "webtransport" && sessionPathMatches(wsi);
}

bool LibwebsocketsWebTransportProvider::openSession(lws* session_wsi)
{
    if (!sessionPathMatches(session_wsi))
    {
        TraceEvent("session_bad_path", session_wsi, lws_get_parent(session_wsi));
        memolog::LogWarn("webtransport.provider.lws.bad_path",
                         "libwebsockets WebTransport session rejected by path",
                         {{"expected_path", _options.path}});
        return false;
    }

    auto state = std::make_shared<SessionState>();
    state->session_wsi = session_wsi;
    auto weak_state = std::weak_ptr<SessionState>(state);
    auto session = _hooks.open_session(
        [this, weak_state](std::vector<std::uint8_t> frame)
        {
            return sendFrame(weak_state, std::move(frame));
        },
        [this](const std::string& session_id)
        {
            requestClose(session_id);
        });
    if (!session)
    {
        return false;
    }

    state->session = session;
    state->session_id = session->sessionId();

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions_by_id.emplace(state->session_id, state);
        _states_by_wsi.emplace(session_wsi, state);
    }

    TraceEvent("session_open", session_wsi, lws_get_parent(session_wsi), 0, state->session_id);
    return true;
}

bool LibwebsocketsWebTransportProvider::attachStream(lws* stream_wsi)
{
    std::shared_ptr<SessionState> state;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        lws* parent = lws_get_parent(stream_wsi);
        state = stateForWsiLocked(parent);
        if (!state || state->closed)
        {
            TraceEvent("stream_attach_no_parent_session", stream_wsi, parent);
            return false;
        }
        if (state->stream_wsi != nullptr && state->stream_wsi != stream_wsi)
        {
            memolog::LogWarn("webtransport.provider.lws.extra_stream_rejected",
                             "libwebsockets WebTransport extra stream rejected",
                             {{"session_id", state->session_id}});
            return false;
        }
        state->stream_wsi = stream_wsi;
        _states_by_wsi.emplace(stream_wsi, state);
        if (!state->outbound.empty())
        {
            lws_callback_on_writable(stream_wsi);
        }
        TraceEvent("stream_attach", stream_wsi, parent, 0, state->session_id);
    }
    return true;
}

bool LibwebsocketsWebTransportProvider::sendFrame(std::weak_ptr<SessionState> weak_state,
                                                  std::vector<std::uint8_t> frame)
{
    auto state = weak_state.lock();
    if (!state)
    {
        return false;
    }

    lws_context* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (state->closed || state->close_requested)
        {
            TraceEvent("send_rejected", state->stream_wsi, state->session_wsi, frame.size(), state->session_id);
            return false;
        }
        state->outbound.push_back(std::move(frame));
        TraceEvent("send_enqueue",
                   state->stream_wsi,
                   state->session_wsi,
                   state->outbound.back().size(),
                   state->session_id);
        context = _context;
    }
    if (context != nullptr)
    {
        lws_cancel_service(context);
    }
    return true;
}

int LibwebsocketsWebTransportProvider::writeFrameToWsi(lws* wsi,
                                                       const std::vector<std::uint8_t>& frame,
                                                       const std::string& session_id,
                                                       const char* event,
                                                       const char* ok_event)
{
    TraceEvent(event, wsi, lws_get_parent(wsi), frame.size(), session_id);
    std::vector<unsigned char> buffer(LWS_PRE + frame.size());
    std::copy(frame.begin(), frame.end(), buffer.begin() + LWS_PRE);
    const int written =
        lws_write(wsi, buffer.data() + LWS_PRE, static_cast<unsigned int>(frame.size()), LWS_WRITE_BINARY);
    if (written == static_cast<int>(frame.size()))
    {
        TraceEvent(ok_event, wsi, lws_get_parent(wsi), frame.size(), session_id);
    }
    else
    {
        TraceEvent("write_deferred", wsi, lws_get_parent(wsi), frame.size(), session_id);
    }
    return written;
}

bool LibwebsocketsWebTransportProvider::sessionPathMatches(lws* session_wsi) const
{
    const std::string configured_path = _options.path.empty() ? "/chat" : _options.path;
    std::string request_path = HeaderValue(session_wsi, WSI_TOKEN_HTTP_COLON_PATH);
    if (request_path.empty())
    {
        request_path = HeaderValue(session_wsi, WSI_TOKEN_CONNECT);
    }
    if (request_path.empty())
    {
        return false;
    }
    return StripQuery(request_path) == std::string_view(configured_path);
}

void LibwebsocketsWebTransportProvider::requestClose(const std::string& session_id)
{
    lws_context* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto found = _sessions_by_id.find(session_id);
        if (found == _sessions_by_id.end())
        {
            return;
        }
        found->second->close_requested = true;
        context = _context;
    }
    if (context != nullptr)
    {
        lws_cancel_service(context);
    }
}

void LibwebsocketsWebTransportProvider::requestWritableForPendingLocked()
{
    for (const auto& [_, state] : _sessions_by_id)
    {
        if (!state || state->closed || state->outbound.empty())
        {
            continue;
        }
        ensureWritableStreamLocked(state);
        if (state->stream_wsi != nullptr)
        {
            TraceEvent("request_writable",
                       state->stream_wsi,
                       state->session_wsi,
                       state->outbound.front().size(),
                       state->session_id);
            lws_callback_on_writable(state->stream_wsi);
        }
    }
}

void LibwebsocketsWebTransportProvider::ensureWritableStreamLocked(const std::shared_ptr<SessionState>& state)
{
    if (!state || state->closed || state->stream_wsi != nullptr || state->session_wsi == nullptr)
    {
        return;
    }

    state->stream_wsi = lws_wt_create_stream(state->session_wsi, 0);
    if (state->stream_wsi == nullptr)
    {
        state->close_requested = true;
        memolog::LogWarn("webtransport.provider.lws.stream_create_failed",
                         "libwebsockets WebTransport bidi stream creation failed",
                         {{"session_id", state->session_id}});
        return;
    }
    _states_by_wsi.emplace(state->stream_wsi, state);
}

void LibwebsocketsWebTransportProvider::applyPendingClosesLocked()
{
    for (const auto& [_, state] : _sessions_by_id)
    {
        if (!state || !state->close_requested || state->closed)
        {
            continue;
        }
        if (state->stream_wsi != nullptr)
        {
            lws_set_timeout(state->stream_wsi, PENDING_TIMEOUT_KILLED_BY_SSL_INFO, LWS_TO_KILL_ASYNC);
        }
        if (state->session_wsi != nullptr)
        {
            lws_set_timeout(state->session_wsi, PENDING_TIMEOUT_KILLED_BY_SSL_INFO, LWS_TO_KILL_ASYNC);
        }
    }
}

void LibwebsocketsWebTransportProvider::closeStateLocked(const std::shared_ptr<SessionState>& state)
{
    if (!state || state->closed)
    {
        return;
    }
    state->closed = true;
    state->outbound.clear();
    if (state->session_wsi != nullptr)
    {
        _states_by_wsi.erase(state->session_wsi);
        state->session_wsi = nullptr;
    }
    if (state->stream_wsi != nullptr)
    {
        _states_by_wsi.erase(state->stream_wsi);
        state->stream_wsi = nullptr;
    }
    _sessions_by_id.erase(state->session_id);
}

void LibwebsocketsWebTransportProvider::removeWsiLocked(lws* wsi)
{
    _states_by_wsi.erase(wsi);
}

std::shared_ptr<LibwebsocketsWebTransportProvider::SessionState>
LibwebsocketsWebTransportProvider::stateForWsiLocked(lws* wsi) const
{
    const auto found = _states_by_wsi.find(wsi);
    if (found == _states_by_wsi.end())
    {
        return nullptr;
    }
    return found->second;
}

void LibwebsocketsWebTransportProvider::serviceLoop()
{
    while (!_stopping.load(std::memory_order_acquire) && _context != nullptr)
    {
        const int result = lws_service(_context, kServicePollMs);
        if (result < 0)
        {
            break;
        }
    }
}

} // namespace memochat::chatserver

#endif // MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
