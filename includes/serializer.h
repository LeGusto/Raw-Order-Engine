#pragma once

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <bit>
#include <arpa/inet.h>
#include <cstring>

uint64_t pack754(long double f, uint32_t bits, uint32_t exp_bits);
long double unpack754(uint64_t encoded_f, uint32_t bits, uint32_t exp_bits);
uint64_t htonll(uint64_t val);
uint64_t ntohll(uint64_t val);

template <typename T>
concept Serializable = requires(T t) {
    { t.fields() };
};

template <typename T>
void pack(std::string &buf, const T &val)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            uint64_t packed = htonll(pack754(val, 32, 8));
            buf.append(reinterpret_cast<const char *>(&packed), 8);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            uint64_t packed = htonll(pack754(val, 64, 11));
            buf.append(reinterpret_cast<const char *>(&packed), 8);
        }
    }
    else if constexpr (std::is_enum_v<T>)
    {
        pack(buf, static_cast<std::underlying_type_t<T>>(val));
    }
    else if constexpr (std::is_integral_v<T>)
    {
        if constexpr (sizeof(T) == 1)
        {
            int8_t new_val = val; // just a single byte, no order
            buf.append(reinterpret_cast<const char *>(&new_val), 1);
        }
        else if constexpr (sizeof(T) == 2)
        {
            int16_t new_val = htons(val);
            buf.append(reinterpret_cast<const char *>(&new_val), 2);
        }
        else if constexpr (sizeof(T) == 4)
        {
            int32_t new_val = htonl(val);
            buf.append(reinterpret_cast<const char *>(&new_val), 4);
        }
        else if constexpr (sizeof(T) == 8)
        {
            int64_t new_val = htonll(val);
            buf.append(reinterpret_cast<const char *>(&new_val), 8);
        }
    }
    else if constexpr (Serializable<T>)
    {
        std::apply([&](const auto &...field)
                   { (pack(buf, field), ...); }, val.fields());
    }
    else if constexpr (requires { val.begin(); val.size(); })
    {
        pack(buf, static_cast<uint32_t>(val.size()));
        for (const auto &elem : val)
            pack(buf, elem);
    }
    else
    {
        throw std::runtime_error("Unsupported type, cannot pack");
    }
}

template <typename T>
void unpack(const std::string &buf, size_t &offset, T &field)
{
    if constexpr (std::is_integral_v<T>)
    {
        std::memcpy(&field, buf.data() + offset, sizeof(T));
        offset += sizeof(T);

        if constexpr (sizeof(T) == 2)
        {
            field = ntohs(field);
        }
        else if constexpr (sizeof(T) == 4)
        {
            field = ntohl(field);
        }
        else if constexpr (sizeof(T) == 8)
        {
            field = ntohll(field);
        }
    }
    else if constexpr (std::is_enum_v<T>)
    {
        std::underlying_type_t<T> tmp;
        unpack(buf, offset, tmp);
        field = static_cast<T>(tmp);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        if constexpr (std::is_same_v<float, T>)
        {
            uint64_t tmp;
            std::memcpy(&tmp, buf.data() + offset, sizeof(T));
            offset += sizeof(T);

            tmp = ntohll(tmp);
            field = unpack754(tmp, 32, 8);
        }
        else if constexpr (std::is_same_v<double, T>)
        {
            uint64_t tmp;
            std::memcpy(&tmp, buf.data() + offset, sizeof(T));
            offset += sizeof(T);

            tmp = ntohll(tmp);
            field = unpack754(tmp, 64, 11);
        }
    }
    else if constexpr (requires { field.begin(); field.size(); })
    {
        uint32_t count;
        unpack(buf, offset, count);

        field.resize(count);

        for (auto &v : field)
        {
            unpack(buf, offset, v);
        }
    }
    else if constexpr (Serializable<T>)
    {
        std::apply([&](auto &...f)
                   { (unpack(buf, offset, f), ...); }, field.fields());
    }
}
