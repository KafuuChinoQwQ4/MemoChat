#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
#include <meta>
#endif

namespace memochat::reflection
{

namespace detail
{

template <typename>
inline constexpr bool always_false_v = false;

} // namespace detail

#if MEMOCHAT_ENABLE_CPP26_REFLECTION

template <typename T> consteval std::size_t FieldCount()
{
    return std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<T>, std::meta::access_context::current()).size();
}

template <typename T, std::size_t I> consteval std::string_view FieldName()
{
    static_assert(I < FieldCount<T>(), "Field index is out of range");
    auto members =
        std::meta::nonstatic_data_members_of(^^std::remove_cvref_t<T>, std::meta::access_context::current());
    return std::meta::identifier_of(members[I]);
}

template <typename T, std::size_t... I>
consteval auto FieldNamesImpl(std::index_sequence<I...>) -> std::array<std::string_view, sizeof...(I)>
{
    return {FieldName<T, I>()...};
}

template <typename T> consteval auto FieldNames()
{
    return FieldNamesImpl<T>(std::make_index_sequence<FieldCount<T>()>{});
}

template <typename T, std::size_t N> consteval bool FieldNamesEqual(const std::array<std::string_view, N>& expected)
{
    constexpr auto actual = FieldNames<T>();
    if constexpr (actual.size() != N)
    {
        return false;
    }
    else
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            if (actual[i] != expected[i])
            {
                return false;
            }
        }
        return true;
    }
}

#else

template <typename T> consteval std::size_t FieldCount()
{
    static_assert(detail::always_false_v<T>, "C++26 reflection is disabled for this target");
    return 0;
}

template <typename T, std::size_t I> consteval std::string_view FieldName()
{
    (void) I;
    static_assert(detail::always_false_v<T>, "C++26 reflection is disabled for this target");
    return {};
}

template <typename T> consteval auto FieldNames()
{
    static_assert(detail::always_false_v<T>, "C++26 reflection is disabled for this target");
    return std::array<std::string_view, 0>{};
}

template <typename T, std::size_t N> consteval bool FieldNamesEqual(const std::array<std::string_view, N>&)
{
    static_assert(detail::always_false_v<T>, "C++26 reflection is disabled for this target");
    return false;
}

#endif

} // namespace memochat::reflection
