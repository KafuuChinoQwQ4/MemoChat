#pragma once

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace memochat::random
{
namespace detail
{
inline void SetUuidError(std::string* error, std::string message) noexcept
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

inline bool FillSecureBytes(std::array<std::uint8_t, 16>& bytes, std::string* error) noexcept
{
#ifdef _WIN32
    const NTSTATUS status = ::BCryptGenRandom(nullptr,
                                              reinterpret_cast<PUCHAR>(bytes.data()),
                                              static_cast<ULONG>(bytes.size()),
                                              BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status < 0)
    {
        SetUuidError(error, "BCryptGenRandom failed with status " + std::to_string(status));
        return false;
    }
    return true;
#else
#ifdef O_CLOEXEC
    constexpr int open_flags = O_RDONLY | O_CLOEXEC;
#else
    constexpr int open_flags = O_RDONLY;
#endif
    const int descriptor = ::open("/dev/urandom", open_flags);
    if (descriptor < 0)
    {
        SetUuidError(error, std::string("open /dev/urandom failed: ") + std::strerror(errno));
        return false;
    }

    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const auto count = ::read(descriptor, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            const int read_error = count < 0 ? errno : EIO;
            ::close(descriptor);
            SetUuidError(error, std::string("read /dev/urandom failed: ") + std::strerror(read_error));
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }

    if (::close(descriptor) != 0)
    {
        SetUuidError(error, std::string("close /dev/urandom failed: ") + std::strerror(errno));
        return false;
    }
    return true;
#endif
}
} // namespace detail

inline bool GenerateUuid(std::string& value, std::string* error = nullptr) noexcept
{
    value.clear();
    if (error != nullptr)
    {
        error->clear();
    }

    std::array<std::uint8_t, 16> bytes{};
    if (!detail::FillSecureBytes(bytes, error))
    {
        return false;
    }
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0fU) | 0x40U);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3fU) | 0x80U);

    constexpr char digits[] = "0123456789abcdef";
    std::array<char, 36> formatted{};
    std::size_t output = 0;
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        if (index == 4 || index == 6 || index == 8 || index == 10)
        {
            formatted[output++] = '-';
        }
        formatted[output++] = digits[bytes[index] >> 4U];
        formatted[output++] = digits[bytes[index] & 0x0fU];
    }
    value.assign(formatted.data(), formatted.size());
    return true;
}

} // namespace memochat::random
