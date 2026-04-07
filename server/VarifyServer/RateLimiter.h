#pragma once

#include <string>

namespace varifyservice {

class RateLimiter {
public:
    enum class Result { Allowed, RateLimited, Error };

    static Result CheckEmail(const std::string& email, int window_sec, int max_requests);
    static Result CheckIP(const std::string& ip, int window_sec, int max_requests);

private:
    static Result Check(const std::string& key, int window_sec, int max_requests);
};

} // namespace varifyservice
