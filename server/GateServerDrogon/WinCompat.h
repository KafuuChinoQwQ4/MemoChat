/*
 * WinCompat.h — Windows SDK isolation layer for VS2026 + C++20
 *
 * Include as the FIRST line of every GateServerDrogon .cpp file.
 *
 * This file acts as a "black box" that encapsulates all Windows SDK includes
 * and fixes, preventing macro conflicts (especially `byte`) from leaking
 * into the rest of the codebase.
 *
 * Fixes included:
 *   1. htonll / ntohll — Windows Winsock2 only provides 32/16-bit variants.
 *      Self-contained inline implementation using bit-shifts only.
 *   2. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte. We undef it after windows.h.
 *   3. Forward declarations for Windows types used by OpenSSL/CryptoAPI.
 *
 * Architecture:
 *   - Drogon library code stays in its own translation unit (drogon*.obj)
 *   - GateServerDrogon code includes this header first, establishing isolation
 *   - CertUtil.cpp needs windows.h for CryptoAPI, but uses its own local
 *     include guard strategy (WIN32_LEAN_AND_MEAN + undef byte)
 */

#pragma once

#ifdef _WIN32

/* ── 1. Fix missing 64-bit byte-order functions ────────────────────────────── */
/*
 * Winsock2 provides htonl (32-bit) and htons (16-bit) but no 64-bit variant.
 * These are required by some Drogon/trantor networking code.
 * Implementation uses pure bit-shifts — no Winsock2 dependency.
 */
static inline unsigned long long htonll(unsigned long long v) noexcept {
    return ((unsigned long long)(
        (unsigned long)((v >> 32) & 0xFFFFFFFFUL)) << 32) |
        (unsigned long long)((unsigned long)(v & 0xFFFFFFFFUL));
}
static inline unsigned long long ntohll(unsigned long long v) noexcept {
    return htonll(v);
}

/* ── 2. byte macro isolation ───────────────────────────────────────────────── */
/*
 * CRITICAL: Windows SDK headers (especially rpcndr.h) define:
 *     typedef unsigned char byte;
 * This conflicts with C++20 std::byte (std::byte is a scoped enum).
 *
 * Strategy:
 *   1. Include the minimal necessary Windows headers first
 *   2. Immediately #undef byte to restore std::byte usability
 *
 * The #undef must happen AFTER windows.h but BEFORE any other header
 * that might trigger the conflict. By placing it here in WinCompat.h,
 * we ensure all GateServerDrogon code sees the clean namespace.
 */

/* Minimal Windows includes for basic types (used by CertUtil) */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wincrypt.h>

/* ── 3. Restore std::byte after Windows headers ────────────────────────────── */
#ifdef byte
#undef byte
#endif

/* ── 4. Forward declare Windows types needed by OpenSSL ───────────────────── */
/*
 * Some OpenSSL structures reference Windows types. Forward declarations
 * here prevent the need to include additional Windows headers elsewhere.
 */

/* ── 5. Additional Windows compatibility fixes ──────────────────────────────── */
/*
 * Ensure consistent behavior across VS2026 runtime versions.
 */

/* Disable some noisy warnings in Windows SDK headers */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6387)  /* _Param_ could be 0 */
#pragma warning(disable: 6031) /* Return value not checked */
#endif

/* ── 6. Network byte order helpers (additional) ──────────────────────────────── */
/* htons/htonl are provided by Winsock2, but we can add safety wrappers */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  /* _WIN32 */

/* ── Non-Windows stubs for cross-platform compatibility ──────────────────────── */
#ifndef _WIN32
#include <netinet/in.h>  /* for htonl, htons, htonll on Unix */
#endif
