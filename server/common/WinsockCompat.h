/*
 * WinsockCompat.h — Winsock2 isolation layer for MemoChat server
 *
 * Include as the FIRST line of every server .cpp file that uses Winsock2 APIs.
 * Use this INSTEAD OF direct <winsock2.h> includes.
 *
 * Fixes:
 *   1. byte macro conflict — rpcndr.h defines `typedef unsigned char byte;`
 *      which conflicts with C++20 std::byte.
 *   2. NOMINMAX to prevent windows.h from defining min/max macros.
 *   3. Automatic linking of required libraries on MSVC.
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
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "bcrypt.lib")

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

#else  /* !_WIN32 */

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close

#endif  /* _WIN32 */
