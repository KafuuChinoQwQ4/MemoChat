#pragma once

#include <string>

namespace varifyservice {

class EmailSender {
public:
    static bool Send(const std::string& to_email, const std::string& code);
};

} // namespace varifyservice
