#pragma once

#include <string_view>
#include <utility>
#include <array>
#include <cstddef>

template <auto T>
std::string_view extract_name()
{
    std::string_view name = __PRETTY_FUNCTION__;

    auto start = name.find("T = ") + 4;
    auto end = name.find_first_of(";]", start);
    std::string_view value = name.substr(start, end - start);

    auto colons = value.rfind("::");
    if (colons == std::string_view::npos)
        return {};
    return value.substr(colons + 2);
}

template <typename T, int... Is>
constexpr auto build_enum_name_cache(std::integer_sequence<int, Is...>)
{
    std::array<std::string_view, sizeof...(Is)> a{};
    ((a[Is] = extract_name<static_cast<T>(Is)>()), ...);
    return a;
}

template <int Max = 256, typename T>
std::string_view enum_name(T e)
{
    static const auto cache = build_enum_name_cache<T>(std::make_integer_sequence<int, Max>{});
    return cache[static_cast<size_t>(e)];
}
