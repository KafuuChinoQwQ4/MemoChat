// Prefix header to fix VS2026 + Windows SDK byte ambiguity
// MUST be included BEFORE any Windows SDK headers (rpcndr.h, wtypes.h, etc.)
// This defines std::byte before Windows headers can pollute the namespace
#include <cstddef>
