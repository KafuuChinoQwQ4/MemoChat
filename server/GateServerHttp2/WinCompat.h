/*
 * WinCompat.h — Windows SDK isolation layer for VS2026 + C++20
 *
 * Include as the FIRST line of every GateServerHttp2 .cpp file.
 *
 * Fixes:
 *   1. htonll / ntohll — Winsock2 may or may not provide 64-bit variants
 *      depending on Windows SDK version. We provide portable fallbacks.
 *   2. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte.
 */

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

// htonll/ntohll are already defined by ws2def.h (part of winsock2.h).
// We must NOT redefine them — remove our definitions entirely.

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
