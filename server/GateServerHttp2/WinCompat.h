/*
 * WinCompat.h — Windows SDK isolation layer for VS2026 + C++20
 *
 * Include as the FIRST line of every GateServerHttp2 .cpp file.
 *
 * Fixes:
 *   1. htonll / ntohll — Winsock2 only provides 32/16-bit variants.
 *   2. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte.
 */

#pragma once

#ifdef _WIN32

static inline unsigned long long htonll(unsigned long long v) noexcept {
    return ((unsigned long long)(
        (unsigned long)((v >> 32) & 0xFFFFFFFFUL)) << 32) |
        (unsigned long long)((unsigned long)(v & 0xFFFFFFFFUL));
}
static inline unsigned long long ntohll(unsigned long long v) noexcept {
    return htonll(v);
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef byte
#undef byte
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6387)
#pragma warning(disable: 6031)
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  /* _WIN32 */

#ifndef _WIN32
#include <netinet/in.h>
#endif
