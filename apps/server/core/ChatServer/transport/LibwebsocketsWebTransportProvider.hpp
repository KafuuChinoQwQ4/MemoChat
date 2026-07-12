#pragma once

#include "IWebTransportProvider.hpp"
#include "runtime/ExplicitThread.hpp"

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER
#include <libwebsockets.h>
#endif

struct lws;
struct lws_context;

namespace memochat::chatserver
{

class LibwebsocketsWebTransportProvider final : public IWebTransportProvider
{
public:
    LibwebsocketsWebTransportProvider();
    ~LibwebsocketsWebTransportProvider() override;

    bool Start(const WebTransportListenOptions& options,
               WebTransportProviderSessionHooks hooks,
               std::string* error) override;
    void Stop() override;
    bool isRunning() const override;
    std::string providerName() const override;

    static int Callback(lws* wsi, enum lws_callback_reasons reason, void* user, void* in, std::size_t len);

private:
    struct SessionState;

    int handleCallback(lws* wsi, enum lws_callback_reasons reason, void* user, void* in, std::size_t len);
    int handleEstablishedWsi(lws* wsi);
    bool isSessionWsi(lws* wsi) const;
    bool openSession(lws* session_wsi);
    bool attachStream(lws* stream_wsi);
    bool sendFrame(std::weak_ptr<SessionState> weak_state, std::vector<std::uint8_t> frame);
    bool sessionPathMatches(lws* session_wsi) const;
    void requestClose(const std::string& session_id);
    int writeFrameToWsi(lws* wsi,
                        const std::vector<std::uint8_t>& frame,
                        const std::string& session_id,
                        const char* event,
                        const char* ok_event);
    void requestWritableForPendingLocked();
    void ensureWritableStreamLocked(const std::shared_ptr<SessionState>& state);
    void applyPendingClosesLocked();
    void closeStateLocked(const std::shared_ptr<SessionState>& state);
    void removeWsiLocked(lws* wsi);
    std::shared_ptr<SessionState> stateForWsiLocked(lws* wsi) const;
    void serviceLoop();

    WebTransportListenOptions _options;
    WebTransportProviderSessionHooks _hooks;
    lws_context* _context = nullptr;
    memochat::runtime::ExplicitThread _service_thread;
    std::atomic_bool _running{false};
    std::atomic_bool _stopping{false};
    mutable std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<SessionState>> _sessions_by_id;
    std::unordered_map<lws*, std::shared_ptr<SessionState>> _states_by_wsi;
};

} // namespace memochat::chatserver
