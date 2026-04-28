#include "quic_lib.hpp"

namespace memochat {
namespace loadtest {

QuicLibrary& QuicLibrary::instance() {
    static QuicLibrary lib;
    return lib;
}

QuicLibrary::~QuicLibrary() {
    shutdown();
}

bool QuicLibrary::init() {
    if (initialized_.load()) return true;

#if defined(_WIN32) && defined(MEMOCHAT_ENABLE_MSQUIC)
    const QUIC_API_TABLE* api_ptr = nullptr;
    QUIC_STATUS status = MsQuicOpen2(&api_ptr);
    api_ = api_ptr;
    if (!QUIC_SUCCEEDED(status) || api_ == nullptr) {
        last_error_ = "MsQuicOpen2 failed: " + std::to_string(status);
        return false;
    }

    QUIC_REGISTRATION_CONFIG regConfig = {
        "MemoChatLoadTest",
        QUIC_EXECUTION_PROFILE_LOW_LATENCY
    };

    const QUIC_API_TABLE* api = static_cast<const QUIC_API_TABLE*>(api_);
    status = api->RegistrationOpen(&regConfig, &registration_);
    if (!QUIC_SUCCEEDED(status)) {
        last_error_ = "RegistrationOpen failed: " + std::to_string(status);
        MsQuicClose(api);
        api_ = nullptr;
        initialized_ = false;
        return false;
    }
#endif

    initialized_ = true;
    return true;
}

void QuicLibrary::shutdown() {
#if defined(_WIN32) && defined(MEMOCHAT_ENABLE_MSQUIC)
    const QUIC_API_TABLE* api = static_cast<const QUIC_API_TABLE*>(api_);
    if (registration_ && api) {
        api->RegistrationClose(registration_);
        registration_ = nullptr;
    }
    if (api_) {
        MsQuicClose(api);
        api_ = nullptr;
    }
#endif
    initialized_ = false;
}

} // namespace loadtest
} // namespace memochat
