#pragma once
#ifndef MEMOCHAT_LOADTEST_CONFIG_H
#define MEMOCHAT_LOADTEST_CONFIG_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace memochat {
namespace loadtest {

struct TestConfig {
    // HTTP gate
    std::string gate_url = "http://127.0.0.1:8080";
    std::string login_path = "/user_login";
    std::string client_ver = "2.0.0";
    bool use_xor_passwd = true;

    // Accounts
    std::string accounts_csv;
    int min_accounts = 2;

    // Load test params
    int total = 500;
    int concurrency = 50;
    double http_timeout_sec = 5.0;
    double tcp_timeout_sec = 5.0;
    double quic_timeout_sec = 5.0;
    int max_retries = 2;

    // Warmup
    int warmup_iterations = 2;

    // QUIC
    std::string quic_alpn = "memochat-chat";
    std::string cert_file;

    // Logging
    std::string log_dir = "logs";
    std::string report_dir = "reports";
    std::string log_level = "INFO";

    // Protocol
    int protocol_version = 3;
};

struct Account {
    std::string email;
    std::string password;
    std::string uid;
    std::string user_id;
    std::string user;
};

bool load_config(const std::string& path, TestConfig& out);
std::vector<Account> load_accounts_csv(const std::string& path);
std::string xor_encode(const std::string& raw);

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_CONFIG_H
