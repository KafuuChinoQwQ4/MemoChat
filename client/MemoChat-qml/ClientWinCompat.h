/*
 * ClientWinCompat.h — Windows SDK isolation layer for MemoChat QML client
 *
 * Include as the FIRST line of every client .cpp file that uses Windows APIs.
 *
 * Fixes:
 *   1. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte.
 *   2. NOMINMAX to prevent windows.h from defining min/max macros.
 */

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <dwmapi.h>

/* Restore std::byte after Windows headers */
#ifdef byte
#undef byte
#endif

/* Restore NOMINMAX side-effect */
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#endif  /* _WIN32 */
