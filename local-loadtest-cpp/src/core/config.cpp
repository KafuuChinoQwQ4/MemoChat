#include "config.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

namespace memochat {
namespace loadtest {

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace((unsigned char)*start)) ++start;
    auto end = s.end();
    while (end != start && std::isspace((unsigned char)*(end - 1))) --end;
    return std::string(start, end);
}

static std::string get_env_or(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : fallback;
}

static std::string resolve_path(const std::string& base_dir, const std::string& path) {
    if (path.empty()) return path;
    if (fs::path(path).is_absolute()) return fs::path(path).string();
    return (fs::path(base_dir) / path).string();
}

bool load_config(const std::string& path, TestConfig& out) {
    std::ifstream f(path);
    if (!f) return false;

    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    // Simple JSON parser for known keys
    // Gate
    auto kv = [&](const std::string& key, auto& var, bool is_str = true) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = content.find(pattern);
        if (pos == std::string::npos) return;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return;
        size_t quote1 = content.find('"', colon);
        if (quote1 == std::string::npos) return;
        size_t quote2 = content.find('"', quote1 + 1);
        if (quote2 == std::string::npos) return;
        std::string val = trim(content.substr(quote1 + 1, quote2 - quote1 - 1));
        if constexpr (std::is_same_v<decltype(var), std::string&>) {
            var = val;
        } else if constexpr (std::is_same_v<decltype(var), bool&>) {
            var = (val == "true");
        } else {
            if (val.find('.') != std::string::npos) {
                var = (double)std::stod(val);
            } else {
                var = (int)std::stoi(val);
            }
        }
    };

    kv("gate_url", out.gate_url);
    kv("gate_url_https", out.gate_url_https);
    kv("gate_url_http3", out.gate_url_http3);
    kv("accounts_csv", out.accounts_csv);
    kv("login_path", out.login_path);
    kv("client_ver", out.client_ver);
    kv("use_xor_passwd", out.use_xor_passwd);
    kv("quic_alpn", out.quic_alpn);
    kv("log_dir", out.log_dir);
    kv("report_dir", out.report_dir);
    kv("log_level", out.log_level);

    auto ki = [&](const std::string& section, const std::string& key, auto& var) {
        std::string full = "\"" + section + "\"";
        size_t spos = content.find(full);
        if (spos == std::string::npos) return;
        std::string pattern = "\"" + key + "\"";
        size_t pos = content.find(pattern, spos);
        if (pos == std::string::npos) return;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return;
        size_t end = colon + 1;
        while (end < content.size() && std::isspace((unsigned char)content[end])) ++end;
        if (end >= content.size()) return;
        std::string token;
        if (content[end] == '"') {
            size_t q2 = content.find('"', end + 1);
            token = content.substr(end + 1, q2 - end - 1);
        } else {
            size_t comma = content.find(',', end);
            size_t brace = content.find('}', end);
            size_t tok_end = std::min(comma, brace);
            token = trim(content.substr(end, tok_end - end));
        }
        if constexpr (std::is_same_v<decltype(var), int&>) {
            var = std::stoi(token);
        } else if constexpr (std::is_same_v<decltype(var), double&>) {
            var = std::stod(token);
        } else if constexpr (std::is_same_v<decltype(var), std::string&>) {
            var = token;
        }
    };

    ki("http_login", "total", out.total);
    ki("http_login", "concurrency", out.concurrency);
    ki("http_login", "timeout_sec", out.http_timeout_sec);

    ki("tcp_login", "protocol_version", out.protocol_version);
    ki("tcp_login", "total", out.total);
    ki("tcp_login", "concurrency", out.concurrency);
    ki("tcp_login", "http_timeout_sec", out.http_timeout_sec);
    ki("tcp_login", "tcp_timeout_sec", out.tcp_timeout_sec);

    ki("quic_login", "protocol_version", out.protocol_version);
    ki("quic_login", "total", out.total);
    ki("quic_login", "concurrency", out.concurrency);
    ki("quic_login", "http_timeout_sec", out.http_timeout_sec);
    ki("quic_login", "quic_timeout_sec", out.quic_timeout_sec);
    ki("quic_login", "alpn", out.quic_alpn);

    ki("warmup", "iterations", out.warmup_iterations);

    // Resolve relative paths
    fs::path cfg_dir = fs::path(path).parent_path();

    if (!out.accounts_csv.empty() && !fs::path(out.accounts_csv).is_absolute()) {
        out.accounts_csv = (cfg_dir / out.accounts_csv).string();
    }
    if (!out.log_dir.empty() && !fs::path(out.log_dir).is_absolute()) {
        out.log_dir = (cfg_dir / out.log_dir).string();
    }
    if (!out.report_dir.empty() && !fs::path(out.report_dir).is_absolute()) {
        out.report_dir = (cfg_dir / out.report_dir).string();
    }

    // Env overrides
    out.log_dir = get_env_or("MEMOCHAT_LOADTEST_LOG_DIR", out.log_dir);
    out.report_dir = get_env_or("MEMOCHAT_LOADTEST_REPORT_DIR", out.report_dir);

    return true;
}

std::vector<Account> load_accounts_csv(const std::string& path) {
    std::vector<Account> accounts;
    std::ifstream f(path);
    if (!f) return accounts;

    std::string line;
    std::vector<std::string> headers;
    bool header_read = false;

    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cols;

        while (std::getline(ss, cell, ',')) {
            cols.push_back(trim(cell));
        }
        if (!header_read) {
            headers = cols;
            header_read = true;
            continue;
        }

        Account acc;
        for (size_t i = 0; i < headers.size() && i < cols.size(); ++i) {
            const std::string& h = headers[i];
            const std::string& v = cols[i];
            if (h == "email") acc.email = v;
            else if (h == "password") acc.password = v;
            else if (h == "last_password") {
                if (acc.password.empty()) acc.password = v;
            }
            else if (h == "uid") acc.uid = v;
            else if (h == "user_id") acc.user_id = v;
            else if (h == "user") acc.user = v;
        }
        if (!acc.email.empty() && !acc.password.empty()) {
            accounts.push_back(acc);
        }
    }
    return accounts;
}

std::string xor_encode(const std::string& raw) {
    int x = (int)(raw.size() % 255);
    std::string out;
    out.reserve(raw.size());
    for (char ch : raw) {
        out += char((unsigned char)ch ^ x);
    }
    return out;
}

} // namespace loadtest
} // namespace memochat
