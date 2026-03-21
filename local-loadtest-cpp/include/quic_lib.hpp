#pragma once
#ifndef MEMOCHAT_LOADTEST_QUIC_LIB_H
#define MEMOCHAT_LOADTEST_QUIC_LIB_H

// msquic library singleton — owns registration for the entire process lifetime.
// Lives for the duration of the process; all QuicChatClient instances share it.

#if defined(_WIN32)
#include <winsock2.h>
#include <msquic.h>
#endif
#include <string>
#include <atomic>

namespace memochat {
namespace loadtest {

class QuicLibrary {
public:
    static QuicLibrary& instance();

    // Must be called before any client connects.
    // Idempotent — safe to call multiple times.
    bool init();

    // Call on process shutdown (optional; RAII handles it at static destruction).
    void shutdown();

    // Returns the QUIC_API_TABLE* for use in QuicChatClient.
    // Returns nullptr if not initialized.
    const void* api() const { return api_; }

    HQUIC registration() const { return registration_; }
    bool is_ready() const { return initialized_.load(); }

    // Human-readable last error
    const std::string& last_error() const { return last_error_; }

private:
    QuicLibrary() = default;
    ~QuicLibrary();

    QuicLibrary(const QuicLibrary&) = delete;
    QuicLibrary& operator=(const QuicLibrary&) = delete;

    const void* api_ = nullptr; // QUIC_API_TABLE* (opaque, dereferenced via msquic v2 API)
    HQUIC registration_ = nullptr;
    std::atomic<bool> initialized_{false};
    std::string last_error_;
};

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_QUIC_LIB_H
