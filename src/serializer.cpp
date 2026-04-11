#pragma once

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <bit>
#include <arpa/inet.h>
#include <cstring>

uint64_t pack754(long double f, uint32_t bits, uint32_t exp_bits)
{
    if (f == 0)
        return 0;
    uint32_t bias = (1ULL << (exp_bits - 1)) - 1;
    uint64_t sign = (f < 0);

    long double f_tmp = std::abs(f);

    int32_t exp = 0;
    while (f_tmp >= 2)
    {
        f_tmp /= 2;
        exp++;
    }
    while (f_tmp < 1)
    {
        f_tmp *= 2;
        exp--;
    }

    f_tmp -= 1;

    uint64_t significand_bits = bits - exp_bits - 1;
    long double scaled_f = f_tmp * (1LL << significand_bits);     // convert to long long
    uint64_t significand = static_cast<uint64_t>(scaled_f + 0.5); // rounds to the nearest int
    // std::cout << exp << "buh\n";
    exp += bias;

    uint64_t ans = (sign << (bits - 1));
    ans |= (static_cast<uint64_t>(exp) << (significand_bits));
    ans |= significand;

    // std::cout << sign << " " << exp << " " << significand << "\n";
    // std::cout << ans << " huh\n";
    return ans;
}

long double unpack754(uint64_t encoded_f, uint32_t bits, uint32_t exp_bits)
{
    if (encoded_f == 0)
        return 0;
    // std::cout << encoded_f << "\n";
    uint64_t bias = (1 << (exp_bits - 1)) - 1;
    uint64_t significand_bits = bits - exp_bits - 1;
    uint64_t sign = ((1ULL << (bits - 1)) & encoded_f) > 0;
    uint64_t significand = (encoded_f & ((1ULL << significand_bits) - 1));
    int64_t exp = ((encoded_f >> significand_bits) & ((1 << exp_bits) - 1));
    exp -= bias;

    // std::cout << sign << " " << exp << " " << significand << "\n";

    long double f = static_cast<long double>(significand) / (1LL << significand_bits);
    f += 1;

    while (exp < 0)
    {
        f /= 2;
        exp++;
    }
    while (exp > 0)
    {
        f *= 2;
        exp--;
    }

    if (sign)
        f *= -1;

    return f;
}

uint64_t htonll(uint64_t val)
{
    if constexpr (std::endian::native == std::endian::little)
        return std::byteswap(val);
    return val;
}

uint64_t ntohll(uint64_t val)
{
    if constexpr (std::endian::native == std::endian::little)
        return std::byteswap(val);
    return val;
}

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
    else if constexpr (Serializable<T>)
    {
        std::apply([&](auto &...f)
                   { (unpack(buf, offset, f), ...); }, field.fields());
    }
}
