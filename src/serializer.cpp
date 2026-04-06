#include <cstdint>
#include <algorithm>
#include <iostream>
#include <bit>

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
            uint64_t packed = pack754(val, 32, 8);
            buf.push_back(htonll(packed));
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            uint64_t packed = pack754(val, 64, 11);
            buf.push_back(htonll(packed));
        }
    }
    else if constexpr (tsd::is_integral_v<T>)
    {
        if constexpr (sizeof(T) == 8)
        {
            int8_t new_val = val; // just a single byte, no order
            buf.push_back(val);
        }
        else if constexpr (sizeof(T) == 16)
        {
            int16_t new_val = htons(val);
            buf.push_back(htons(new_val));
        }
        else if constexpr (sizeof(T) == 32)
        {
            int32_t new_val = htonl(val);
            buf.push_back(htonl(new_val));
        }
        else if constexpr (sizeof(T) == 64)
        {
            int64_t new_val = htonll(val);
            buf.push_back(htonll(new_val));
        }
    }
    else if constexpr (Serializable<T>)
    {
        ; //
    }
    else
    {
        throw std::runtime_error("Unsupported type, cannot pack");
    }
}
