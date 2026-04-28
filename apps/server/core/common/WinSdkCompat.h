/*
 * WinSdkCompat.h — Windows SDK isolation layer for MemoChat server
 *
 * Include as the FIRST line of every server .cpp file that uses Windows APIs.
 *
 * Fixes:
 *   1. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte.
 *   2. NOMINMAX to prevent windows.h from defining min/max macros.
 *   3. WIN32_LEAN_AND_MEAN for faster compilation.
 *   4. Winsock2 initialization (WSAStartup) for network code.
 */

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

/* Restore std::byte after Windows headers */
#ifdef byte
#undef byte
#endif

/* Restore min/max after Windows headers */
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <windows.h>

/* Restore std::byte again after windows.h */
#ifdef byte
#undef byte
#endif

#else  /* !_WIN32 */

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#endif  /* _WIN32 */
