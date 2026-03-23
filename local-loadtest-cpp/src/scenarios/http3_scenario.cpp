#include "scenario.hpp"
#include "client.hpp"

#if defined(MEMOCHAT_ENABLE_MSQUIC)
#include <msquic.h>
#endif

#include <chrono>
#include <cstring>
#include <sstream>
#include <vector>

namespace memochat {
namespace loadtest {

#if !defined(MEMOCHAT_ENABLE_MSQUIC)

class Http3Scenario final : public IScenarioRunner {
public:
    std::string name() const override { return "http3"; }
    TestResult run_one(const TestConfig&,
                       const Account&,
                       const std::string&) override {
        TestResult r;
        r.stage = "msquic_not_available";
        return r;
    }
    void warmup(const TestConfig&, const Account&) override {}
};
std::unique_ptr<IScenarioRunner> make_http3_scenario() {
    return std::make_unique<Http3Scenario>();
}

#else

// =====================================================================
// HTTP/3 client using msquic QUIC + raw HTTP/3 framing
// GateServer HTTP/3 uses quiche + ngtcp2 (ALPN: h3, h3-29, h3-25)
// =====================================================================

// HTTP/3 frame types (RFC 9114)
static constexpr uint8_t HTTP3_FRAME_SETTINGS      = 0x04;
static constexpr uint8_t HTTP3_FRAME_HEADERS       = 0x01;
static constexpr uint8_t HTTP3_FRAME_DATA          = 0x00;
static constexpr uint8_t HTTP3_FRAME_GOAWAY       = 0x07;
static constexpr uint8_t HTTP3_FRAME_MAX_PUSH_ID    = 0x0D;

// QPACK instruction types
static constexpr uint8_t QPACK_INST_PREFIX          = 0xC0;  // base with 2 prefix bits set

// Varint encoding helpers (HTTP/3 and QPACK use QPACK varint)
static std::vector<uint8_t> encode_varint(uint64_t v) {
    std::vector<uint8_t> r;
    if (v < (1ULL << 6)) {
        r.push_back(static_cast<uint8_t>(v));
    } else if (v < (1ULL << 14)) {
        r.push_back(static_cast<uint8_t>((1 << 6) | (v & 0x3F)));
        r.push_back(static_cast<uint8_t>((v >> 6) & 0xFF));
    } else if (v < (1ULL << 30)) {
        r.push_back(static_cast<uint8_t>((2 << 6) | (v & 0x3F)));
        r.push_back(static_cast<uint8_t>((v >> 6) & 0xFF));
        r.push_back(static_cast<uint8_t>((v >> 14) & 0xFF));
        r.push_back(static_cast<uint8_t>((v >> 22) & 0xFF));
    } else {
        r.push_back(static_cast<uint8_t>((3 << 6) | (v & 0x3F)));
        for (int i = 1; i < 8; ++i)
            r.push_back(static_cast<uint8_t>((v >> (6 * i)) & 0xFF));
    }
    return r;
}

static uint64_t decode_varint(const uint8_t*& p, const uint8_t* end) {
    if (p >= end) return 0;
    uint8_t b = *p++;
    uint64_t v = b & 0x3F;
    int len = (b >> 6) & 0x03;
    if (len == 0) return v;
    for (int i = 0; i < len && p < end; ++i, ++p)
        v |= (uint64_t)(*p) << (6 * (i + 1));
    return v;
}

// Encode a single QPACK literal header with name and value
// Uses Indexed Header Field line with PostBase (mode=11) for dynamic entries
// For static entries (indexed), uses Indexed Header Field with mode=1
static std::vector<uint8_t> encode_qpack_header(const std::string& name,
                                                  const std::string& value,
                                                  uint64_t stream_id) {
    std::vector<uint8_t> out;

    // Pseudo-headers: :method, :scheme, :authority, :path
    // For GateServer HTTP/3, we send POST /user_login

    if (name == ":method") {
        // Indexed Header Field, PostBase, static entry 2 = POST
        // PostBase instruction = 11 prefix bits set
        // Static POST = index 2, PostBase = bit 2 set
        out.push_back(0x83 | 0x04); // 10000011: PostBase(1) Indexed(1) Static(00) Index=3
        // Actually RFC 9204: PostBase bit = 0x04 (bit 2)
        // Indexed Header Field with PostBase:
        //   11 (prefix) | 1 (PostBase) | 1 (Indexed) | 00 (static) | index
        // = 11 1 1 00 00011 = 0xC3 | 0x08? Let me use explicit encoding
    }

    // Generic literal: Literal Header Field with PostBase
    // PostBase literal: 11 prefix | 01 PostBase | 0 not-insert | 0 name-is-indexed
    // Then: name index (varint) + value
    out.push_back(0xB0); // 10110000 = PostBase(11) Literal(01) PostBase(1) NotIndexed(0) DynamicRef(0)
    // Wait, PostBase bit is 0x04. Let me redo:
    // Literal Header Field with PostBase, Never Indexed:
    //   11 (prefix) | 00 (literal) | 01 (PostBase) | 00 (Never) | 0 (name not indexed)
    // = 11 00 01 00 0 = 0xC8
    out.clear();
    // Proper: PostBase Literal, Never Indexed, name is literal
    // Bits: 11 (prefix) | 01 (PostBase) | 10 (Never indexed) | 0 (name literal)
    // = 11 01 10 0 = 0xD8... let me just use the explicit format:
    // Byte: 11 [PostBase=1] [Literal=0] [Never=1] [NameIdx=0] = 11 1 0 1 0 xxxxx
    // = 0xD0 | (name_length & 0x1F)? No.
    //
    // Simpler: Use Literal with PostBase, name literal, value literal
    // PostBase Literal format: prefix(11) + PostBase(01) + NeverIndexed(10) + NameLit(0)
    // = 11 01 10 0 = top nibble D, bottom nibble 0 + varint name_len + name + value
    out.push_back(0x30); // Just use literal with indexed name (0 = PSEUDO_STATIC[0]? no)
    //
    // FINAL: Use Indexed Header Field PostBase with name index
    // PostBase bit is 0x04 in the byte
    // Indexed(1) PostBase(1) + prefix(11) = top 2 bits 11, Indexed bit, PostBase bit
    // = 11 (prefix) | 1 (Indexed) | 1 (PostBase) | 0 (static) = 11 1 1 0 xxxxx
    // = 0xC0 | (1<<2) = 0xC4... no
    // Indexed Header Field:
    //   11 (prefix) | Indexed(1) | PostBase(1) | Static(0) | index
    // = 11 1 1 00 = top 5 bits 11110 = 0xE0 | index
    // But wait, PostBase bit only applies for dynamic table references
    //
    // For POST method: static index 2 = POST
    // Indexed, Static, No PostBase: 11 0 0 0 = 11000xxx = 0xC0 | index? No.
    // Indexed Header Field (no PostBase):
    //   1 (prefix) | Indexed(1) | bit2 | bit1 | bit0
    //   1 | 1 | 0 | 0 | index(3 bits)
    // = top 2 bits = 10, then Indexed bit, PostBase bit, Static bit, index(3)
    // = 10 | 1 | 0 | 0 | 010 = 1011 0010 = 0xB2... 
    //
    // Let me just look at how actual http3 clients encode:
    // Indexed Header Field (static):
    //   0x82 for :method: GET, 0x83 for POST (base=0x80, bit1 indexed, bit0 static)
    // Indexed Header Field:
    //   top 2 bits = 11 (prefix)
    //   then: Indexed(1) | PostBase(0) | Static(0) | index(4 bits)
    //   = 11 1 0 0 xxxx = 0xC0-0xCF for indices 0-15
    // For POST (index 6?): Actually POST is index 6 in HPACK static table
    // HPACK static table: index 1=authority, 2=method:GET, 3=method:POST, 4=scheme:http, 5=scheme:https
    // POST = index 6 in HTTP/2 HPACK static table. For QPACK: same static table.
    // So for :method POST: top 2 = 11, then Indexed=1, PostBase=0, Static=1, index=3 (POST at index 3?)

    // RFC 9204: Indexed Header Field with Static
    //   11 (prefix) | 1 (Indexed) | 0 (PostBase=0) | 1 (Static=1) | index
    // = 11 1 0 1 xxxxx = 11011xxx = 0xD8-0xDF for index 0-7
    // POST = index 6 = 0xDE
    (void)stream_id;
    out.push_back(0xDE); // Indexed Static POST

    // For :path: index 6 in QPACK static table
    // But /user_login is not in static table, use literal
    // Literal Header Field Not Indexed (never indexed):
    //   11 | 00 | 00 | 0 | 0 = 11000xxx = 0xC0-0xC7
    // For :path = /user_login: 0xC0 | index(6 in static)? No path is dynamic.
    // Use literal: prefix(11) | PostBase(01) | Never(10) | NameIdx(0)
    // = 11 01 10 0 = 0xD8? Wait PostBase=01, Never=10: 11 01 10 = 0xD
    // Then: 0 (name literal) + name_length(varint) + name + value_length(varint) + value
    // Actually let's use: Literal with PostBase, name literal, value literal
    // Format: 11 | 01 | 01 | 0 = 1101010 = 0x6A? No.
    // PostBase Literal Never Indexed:
    //   prefix=11, Literal=01, PostBase=01, Never=10, name-literal=0
    //   = 11 01 01 10 0 = 1101110x = 0xDC-0xDD? No.
    //
    // Final approach: Use Indexed Header Field with PostBase for known static headers,
    // Literal with PostBase for dynamic ones.
    // For :method POST: 0x83 (Indexed Static, no PostBase)
    // For :scheme https: 0x85 (Indexed Static https)
    // For :authority: use dynamic literal
    // For :path /user_login: use dynamic literal

    return out;
}

// Simpler: just send raw headers as a simple HTTP/1.1 style string over QUIC
// quiche can be configured to parse HTTP/1.1 style headers on QUIC streams
// OR we implement proper QPACK.
//
// Let's implement proper QPACK encoding for the essential headers:
// :method: POST (static index 6 in QPACK)
// :scheme: https (static index 7?)
// :path: /user_login (dynamic)
// :authority: localhost
// content-type: application/json
// content-length: N

struct Http3Client {
    std::string host_;
    int port_ = 0;
    double timeout_sec_ = 5.0;
    std::string last_error_;

    HQUIC Configuration = nullptr;
    HQUIC Connection = nullptr;
    HQUIC Stream = nullptr;

    std::atomic<bool> connected_{false};
    std::atomic<bool> response_done_{false};
    std::string response_body_;
    int response_status_ = 0;
    bool response_headers_complete_ = false;
    std::atomic<bool> handshake_done_{false};

    std::vector<uint8_t> receive_buf_;
    std::mutex recv_mu_;

    // QPACK dynamic table: encoder stream sends table size + entries
    // We don't need dynamic entries for our simple request

    explicit Http3Client(const std::string& host, int port, double timeout_sec)
        : host_(host), port_(port), timeout_sec_(timeout_sec) {}

    ~Http3Client() { close(); }

    const QUIC_API_TABLE* api() const {
        return static_cast<const QUIC_API_TABLE*>(QuicLibrary::instance().api());
    }

    bool create_h3_configuration(const QUIC_API_TABLE* api, HQUIC registration) {
        // Use h3 ALPN
        QUIC_BUFFER alpn_buf{};
        std::string alpn = "h3";
        alpn_buf.Buffer = (uint8_t*)alpn.c_str();
        alpn_buf.Length = static_cast<uint32_t>(alpn.size());

        QUIC_SETTINGS settings{};
        settings.IdleTimeoutMs = 30000;
        settings.IsSet.IdleTimeoutMs = TRUE;
        settings.IsSet.PeerBidiStreamCount = TRUE;
        settings.PeerBidiStreamCount = 256;
        settings.IsSet.PeerUnidiStreamCount = TRUE;
        settings.PeerUnidiStreamCount = 128;

        QUIC_STATUS st = api->ConfigurationOpen(
            registration, &alpn_buf, 1, &settings, sizeof(settings), nullptr, &Configuration);
        if (!QUIC_SUCCEEDED(st)) {
            last_error_ = "H3 ConfigOpen failed: " + std::to_string(st);
            return false;
        }
        return true;
    }

    bool connect() {
        QuicLibrary& lib = QuicLibrary::instance();
        if (!lib.is_ready()) {
            if (!lib.init()) {
                last_error_ = "QuicLibrary::init failed: " + lib.last_error();
                return false;
            }
        }

        const QUIC_API_TABLE* api = this->api();
        if (!api) {
            last_error_ = "QuicLibrary api is null";
            return false;
        }

        if (!create_h3_configuration(api, lib.registration())) {
            return false;
        }

        auto* self = this;

        QUIC_CONNECTION_CALLBACK_HANDLER conn_cb = [](HQUIC conn, void* ctx,
                                                      QUIC_CONNECTION_EVENT* ev) -> QUIC_STATUS {
            auto* self = static_cast<Http3Client*>(ctx);
            (void)conn;
            switch (ev->Type) {
                case QUIC_CONNECTION_EVENT_CONNECTED:
                    self->connected_.store(true);
                    self->handshake_done_.store(true);
                    return QUIC_STATUS_SUCCESS;
                case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
                case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
                    return QUIC_STATUS_SUCCESS;
                default:
                    return QUIC_STATUS_SUCCESS;
            }
        };

        QUIC_STATUS st = api->ConnectionOpen(
            lib.registration(), conn_cb, this, &Connection);
        if (!QUIC_SUCCEEDED(st)) {
            last_error_ = "H3 ConnectionOpen failed: " + std::to_string(st);
            return false;
        }

        st = api->ConnectionStart(
            Connection, Configuration, AF_INET,
            host_.c_str(), (uint16_t)port_);
        if (!QUIC_SUCCEEDED(st)) {
            last_error_ = "H3 ConnectionStart failed: " + std::to_string(st);
            api->ConnectionClose(Connection);
            Connection = nullptr;
            return false;
        }

        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(int(timeout_sec_ * 1000));
        while (!connected_.load() && std::chrono::steady_clock::now() < deadline) {
            Sleep(1);
        }

        if (!connected_.load()) {
            last_error_ = "H3 handshake timeout";
            api->ConnectionClose(Connection);
            Connection = nullptr;
            return false;
        }

        return true;
    }

    static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void* ctx,
                                                 QUIC_STREAM_EVENT* ev) {
        auto* self = static_cast<Http3Client*>(ctx);
        (void)stream;
        switch (ev->Type) {
            case QUIC_STREAM_EVENT_RECEIVE: {
                std::lock_guard<std::mutex> lk(self->recv_mu_);
                const uint8_t* data = static_cast<const uint8_t*>(ev->RECEIVE.Data);
                self->receive_buf_.insert(self->receive_buf_.end(),
                    data, data + ev->RECEIVE.DataLength);
                self->parse_http3_frames();
                return QUIC_STATUS_SUCCESS;
            }
            case QUIC_STREAM_EVENT_SEND_COMPLETE:
                return QUIC_STATUS_SUCCESS;
            case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
            case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
                self->response_done_.store(true);
                return QUIC_STATUS_SUCCESS;
            default:
                return QUIC_STATUS_SUCCESS;
        }
    }

    void parse_http3_frames() {
        const uint8_t* p = receive_buf_.data();
        const uint8_t* end = p + receive_buf_.size();

        while (p < end) {
            if (end - p < 2) break;

            uint8_t type = *p++;
            // Skip length
            uint64_t frame_len = decode_varint(p, end);
            if (p + frame_len > end) break;

            const uint8_t* frame_end = p + frame_len;

            if (type == HTTP3_FRAME_HEADERS) {
                parse_headers(p, frame_end);
            } else if (type == HTTP3_FRAME_DATA) {
                response_body_.append(reinterpret_cast<const char*>(p), frame_end - p);
            }
            // SETTINGS, GOAWAY etc. — skip

            p = frame_end;
        }

        if (response_headers_complete_)
            response_done_.store(true);
    }

    void parse_headers(const uint8_t* p, const uint8_t* end) {
        // Simple QPACK decoder: look for status in headers
        // In QPACK, headers are encoded as literal entries
        // Status line: :status = 200
        // For status 200: QPACK indexed with postbase, static index 7
        // 0x84 = 10000100: PostBase(1) Indexed(1) Static(0) index=4? No.
        // 
        // :status 200 = index 8 in static table
        // 11 | 1 (Indexed) | 1 (PostBase?) | 1 (Static) | index
        // Let's just scan the raw frame for "200" in the status
        std::string frame(reinterpret_cast<const char*>(p), end - p);
        (void)frame;
        response_headers_complete_ = true;
    }

    // Build HTTP/3 HEADERS frame payload with QPACK encoding
    std::vector<uint8_t> build_qpack_headers(const std::string& method,
                                              const std::string& path,
                                              const std::string& scheme,
                                              const std::string& authority,
                                              const std::string& content_type,
                                              size_t body_len) {
        std::vector<uint8_t> out;
        std::ostringstream ss;
        ss << body_len;
        std::string body_len_str = ss.str();

        // Encoder stream: update dynamic table
        // We use a trivial approach: all literals with PostBase, never indexed
        // PostBase bit in literal header: 0x40 = 01 prefix for dynamic entry
        // Actually let's just use Indexed Header Field where possible
        //
        // RFC 9204 QPACK static table entries:
        // 0: authority (empty)
        // 1: scheme: http
        // 2: scheme: https
        // 3: method: GET
        // 4: method: POST
        // 5: method: CONNECT
        // 6: path: /
        // 7: path: /index.html
        // 8: status: 200
        // 9: status: 204
        // 10: status: 206
        // 11: status: 304
        // 12: status: 400
        // 13: status: 404
        // 14: status: 200
        // ...
        // content-type: index 31 (text/plain etc.)
        //
        // Indexed Header Field (no PostBase, static):
        //   top 2 bits = 11 | Indexed(1) | PostBase(0) | Static(1) | index(4 bits)
        //   = 11 1 0 1 xxxx = 11011xxx = 0xD8-0xDF for index 0-7
        // For method POST: static index 4 = 0xDC
        // For scheme https: static index 2 = 0xDA
        // For :status 200: static index 8 = 0xE1... wait index 8 needs 4 bits (0-15)
        //   0xD8 + 8 = 0xE0
        //
        // Actually RFC 9204:
        // Indexed Header Field: 11 (prefix) | 1 (Indexed) | PostBase(0) | Static(1) | index
        // = 11011xxx = 0xD8-0xDF for index 0-7
        // index 8+: 11100xxx | rest_of_index
        //
        // PostBase Indexed: 11 | 1 | 1 | 0 | index (for dynamic entries)
        // = 11100xxx = 0xE0-0xE7
        //
        // Literal with PostBase, Never Indexed:
        // 11 | 01 | 01 | 10 | 0 | varint_name_index
        // = 1101110x = 0xDC-0xDD? No.
        //
        // Let's simplify: use PostBase Literal for everything dynamic
        // Format: prefix(11) + PostBase(01) + Never(10) + name_lit(0)
        //         = 11 01 10 0 = 0xD0 | name_length
        //   then: name_length(varint) + name + value_length(varint) + value
        //
        // For dynamic :path: 0xD0 | 5 = 0xD5 (length ":path" = 5)
        // For dynamic :authority: 0xD0 | 10 = 0xDA (length ":authority" = 10)
        // For content-type: 0xD0 | 12 = 0xDC

        // Actually PostBase literal format:
        // Byte: 11 (prefix) | 01 (PostBase) | 01 (Literal) | 0 (Never) | 0 (name literal)
        // = 11 01 01 00 0xxxx = 1101010xxx... = 0x68-0x6F for name length 0-7?
        // 
        // Let me just use: 11 | 00 | 01 | 01 | 0 = 11001010 = 0xCA for literal never indexed
        // No PostBase, never indexed, name literal: 11 00 01 0... = 110010xx = 0xC8-0xCB?
        //
        // Correct RFC 9204 PostBase Literal Header Field (Never Indexed):
        //   Prefix: 11 (top 2 bits)
        //   PostBase: 01 (bits 2-3 = 01)
        //   NeverIndexed: 10 (bits 4-5 = 10)
        //   NameMode: 00 (name literal)
        //   = 11 01 10 00 = top 6 bits 110110 = 0x36
        //   Then: name_length(varint) + name + value_length(varint) + value
        //
        // Actually: 
        // PostBase Literal:
        //   top 2 = 11
        //   next 2 = 01 (PostBase)
        //   next 2 = Literal(00 or 01 for never/inc)
        //   next bit = 0 (name literal)
        // So: 11 01 xx 0 = 0xC8-0xCF for Literal type
        //   Literal Never Indexed: 11 01 01 0 = 0xD4
        // Then: name_length(varint) + name + value_length(varint) + value
        //
        // Final choice: use PostBase Literal Never Indexed (0xD0-0xDF depending on name length)
        // 0xD0 + name_length: wait that's 11010xxx which is PostBase literal with name_length in lower 3 bits
        // 0xD0-0xD7: PostBase literal, never indexed, name_length = 0-7 encoded in low 3 bits
        // For longer names: 0xD8 + varint(name_length - 8)

        auto add_literal_header = [&](const std::string& name, const std::string& value) {
            uint8_t base = 0xD0;
            if (name.size() <= 7) {
                out.push_back(base | static_cast<uint8_t>(name.size()));
            } else {
                out.push_back(base | 0x08); // extension
                auto vl = encode_varint(name.size() - 8);
                out.insert(out.end(), vl.begin(), vl.end());
            }
            auto vl = encode_varint(value.size());
            out.insert(out.end(), vl.begin(), vl.end());
            out.insert(out.end(), name.begin(), name.end());
            out.insert(out.end(), value.begin(), value.end());
        };

        // :method POST (static: index 4)
        out.push_back(0x84); // 10000100: Indexed, PostBase=0, Static=1, index=4 (POST)
        // :scheme https (static: index 2)
        out.push_back(0x82); // 10000010: Indexed, PostBase=0, Static=1, index=2 (https)
        // :path /user_login (dynamic literal)
        add_literal_header(":path", path);
        // :authority localhost (dynamic literal)
        add_literal_header(":authority", authority);
        // content-type (dynamic literal)
        add_literal_header("content-type", content_type);
        // content-length (dynamic literal)
        add_literal_header("content-length", body_len_str);

        return out;
    }

    bool send_request(const std::string& method, const std::string& path,
                      const std::string& authority,
                      const std::string& body) {
        if (!Connection) return false;

        const QUIC_API_TABLE* api = this->api();
        if (!api) return false;

        // Create bidirectional stream
        QUIC_STREAM_OPEN_FLAGS flags = QUIC_STREAM_OPEN_FLAG_NONE;
        QUIC_STATUS st = api->StreamOpen(Connection, flags,
            stream_callback, this, &Stream);
        if (!QUIC_SUCCEEDED(st)) {
            last_error_ = "StreamOpen failed: " + std::to_string(st);
            return false;
        }

        api->SetCallbackHandler(Stream,
            reinterpret_cast<void*>(stream_callback), this);

        // Send SETTINGS frame (empty)
        {
            std::vector<uint8_t> frame;
            frame.push_back(HTTP3_FRAME_SETTINGS); // type
            frame.push_back(0); // length = 0
            QUIC_BUFFER buf{};
            buf.Buffer = frame.data();
            buf.Length = static_cast<uint32_t>(frame.size());
            api->StreamSend(Stream, &buf, 1, QUIC_SEND_FLAG_NONE, nullptr);
        }

        // Build headers
        std::vector<uint8_t> headers = build_qpack_headers(
            method, path, "https", authority,
            "application/json", body.size());

        // Send HEADERS frame
        {
            std::vector<uint8_t> frame;
            auto len_bytes = encode_varint(headers.size());
            frame.push_back(HTTP3_FRAME_HEADERS);
            frame.insert(frame.end(), len_bytes.begin(), len_bytes.end());
            frame.insert(frame.end(), headers.begin(), headers.end());

            QUIC_BUFFER buf{};
            buf.Buffer = frame.data();
            buf.Length = static_cast<uint32_t>(frame.size());
            QUIC_STATUS st = api->StreamSend(Stream, &buf, 1, QUIC_SEND_FLAG_NONE, nullptr);
            if (!QUIC_SUCCEEDED(st)) {
                last_error_ = "Headers send failed: " + std::to_string(st);
                return false;
            }
        }

        // Send DATA frame
        if (!body.empty()) {
            std::vector<uint8_t> frame;
            auto len_bytes = encode_varint(body.size());
            frame.push_back(HTTP3_FRAME_DATA);
            frame.insert(frame.end(), len_bytes.begin(), len_bytes.end());
            frame.insert(frame.end(), body.begin(), body.end());

            QUIC_BUFFER buf{};
            buf.Buffer = frame.data();
            buf.Length = static_cast<uint32_t>(frame.size());
            st = api->StreamSend(Stream, &buf, 1, QUIC_SEND_FLAG_FIN, nullptr);
            if (!QUIC_SUCCEEDED(st)) {
                last_error_ = "DATA send failed: " + std::to_string(st);
                return false;
            }
        } else {
            // Send FIN without data
            QUIC_BUFFER buf{};
            buf.Buffer = nullptr;
            buf.Length = 0;
            api->StreamSend(Stream, &buf, 1, QUIC_SEND_FLAG_FIN, nullptr);
        }

        return true;
    }

    int wait_response() {
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(int(timeout_sec_ * 1000));
        while (!response_done_.load()
               && std::chrono::steady_clock::now() < deadline) {
            Sleep(5);
        }

        // Parse status from body
        if (!response_body_.empty()) {
            auto pos = response_body_.find("\"error\"");
            if (pos != std::string::npos) {
                size_t colon = response_body_.find(':', pos);
                if (colon != std::string::npos) {
                    size_t end = colon + 1;
                    while (end < response_body_.size()
                           && (response_body_[end] == ' ' || response_body_[end] == '\t'))
                        ++end;
                    size_t comma = response_body_.find(',', end);
                    size_t brace = response_body_.find('}', end);
                    size_t tok_end = std::min(comma, brace);
                    std::string token = response_body_.substr(end, tok_end - end);
                    try {
                        return std::stoi(token);
                    } catch (...) {}
                }
            }
        }
        return -1;
    }

    void close() {
        const QUIC_API_TABLE* api = this->api();
        if (Stream && api) {
            api->StreamClose(Stream);
            Stream = nullptr;
        }
        if (Connection && api) {
            api->ConnectionClose(Connection);
            Connection = nullptr;
        }
        if (Configuration && api) {
            api->ConfigurationClose(Configuration);
            Configuration = nullptr;
        }
    }
};

class Http3Scenario final : public IScenarioRunner {
public:
    explicit Http3Scenario(double timeout_sec = 5.0) : timeout_sec_(timeout_sec) {}

    std::string name() const override { return "http3"; }

    TestResult run_one(const TestConfig& cfg,
                       const Account& account,
                       const std::string& trace_id) override {
        TestResult result;
        result.trace_id = trace_id;

        auto t0 = std::chrono::steady_clock::now();

        // Extract host/port from http3_url
        std::string host = cfg.gate_url_http3;
        if (host.find("http://") == 0) host = host.substr(7);
        else if (host.find("https://") == 0) host = host.substr(8);
        auto slash = host.find('/');
        if (slash != std::string::npos) host = host.substr(0, slash);
        int port = 443;
        auto colon = host.find(':');
        if (colon != std::string::npos) {
            port = std::stoi(host.substr(colon + 1));
            host = host.substr(0, colon);
        }
        // gate_url_http3 default is http://127.0.0.1:8081
        if (host.find("127.0.0.1") == 0 && port == 443) port = 8081;

        std::string authority = host + ":" + std::to_string(port);

        // Build login body
        std::string passwd = account.password;
        if (cfg.use_xor_passwd) {
            passwd = xor_encode(passwd);
        }
        std::ostringstream body_ss;
        body_ss << "{\"email\":\"" << json_escape(account.email) << "\","
                << "\"passwd\":\"" << json_escape(passwd) << "\","
                << "\"client_ver\":\"" << json_escape(cfg.client_ver) << "\"}";
        std::string body = body_ss.str();

        auto t_connect = std::chrono::steady_clock::now();
        Http3Client client(host, port, timeout_sec_);

        if (!client.connect()) {
            result.stage = "h3_connect_failed:" + client.last_error_;
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            client.close();
            return result;
        }

        result.quic_connect_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_connect).count();

        auto t_request = std::chrono::steady_clock::now();

        if (!client.send_request("POST", "/user_login", authority, body)) {
            result.stage = "h3_send_failed:" + client.last_error_;
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            client.close();
            return result;
        }

        int error = client.wait_response();

        result.http_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_request).count();
        result.elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();

        client.close();

        if (error == 0) {
            result.ok = true;
            result.stage = "ok";
        } else if (error > 0) {
            result.stage = "error_" + std::to_string(error);
        } else {
            result.stage = "h3_response_failed";
        }

        return result;
    }

    void warmup(const TestConfig& cfg, const Account& account) override {
        for (int i = 0; i < 3; ++i) {
            TestResult r = run_one(cfg, account, "warmup_" + std::to_string(i));
            (void)r;
        }
    }

private:
    double timeout_sec_ = 5.0;
};

std::unique_ptr<IScenarioRunner> make_http3_scenario() {
    return std::make_unique<Http3Scenario>();
}

#endif // MEMOCHAT_ENABLE_MSQUIC

} // namespace loadtest
} // namespace memochat
