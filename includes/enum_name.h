#pragma once

#include <string_view>
#include <utility>

template <auto T>
std::string_view extract_name()
{
    std::string_view name = __PRETTY_FUNCTION__;
    auto start = name.rfind("::") + 2;
    std::string_view rt = name.substr(start);
    return rt;
}

template <auto T, int... V>
std::string_view get_enum(T e, int... V)
{
    if (static_cast<int>(e) == V)
        return extract_name(e);
    else
        ...
}