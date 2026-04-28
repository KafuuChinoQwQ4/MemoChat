#pragma once
#include <string>
#include <cstdint>

namespace gateredis {

bool WarmupLoginCache(const std::string& email, const std::string& pwd);

}
