#pragma once

#include <functional>
#include <string>

namespace memolog {

struct LogConfig {
    std::string level = "info";
    std::string dir = "./logs";
    std::string format = "json";
    bool to_console = true;
    std::string rotate_mode = "daily";
    int max_files = 14;
    int max_size_mb = 100;
    bool redact = true;
    std::string env = "local";

    static LogConfig FromGetter(
        const std::function<std::string(const std::string&, const std::string&)>& getter);
};

} // namespace memolog
