#pragma once

#include <string_view>
#include <utility>

template <auto V>
constexpr std::string_view extract_enum_name()
{
    std::string_view s = __PRETTY_FUNCTION__;

    auto start = s.find("V = ") + 4;
    auto end = s.find(']', start);
    auto full = s.substr(start, end - start);

    auto colons = full.rfind("::");
    if (colons == std::string_view::npos)
        return {};
    return full.substr(colons + 2);
}

template <typename E, int... Is>
constexpr std::string_view enum_name_dispatch(E value, std::integer_sequence<int, Is...>)
{
    std::string_view result;
    ((static_cast<E>(Is) == value
          ? (result = extract_enum_name<static_cast<E>(Is)>(), true)
          : false) ||
     ...);
    return result;
}

template <int Max = 256, typename E>
constexpr std::string_view enum_name(E value)
{
    return enum_name_dispatch(value, std::make_integer_sequence<int, Max>{});
}
