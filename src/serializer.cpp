#include "serializer.h"

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
    long double scaled_f = f_tmp * (1LL << significand_bits);
    uint64_t significand = static_cast<uint64_t>(scaled_f + 0.5);
    exp += bias;

    uint64_t ans = (sign << (bits - 1));
    ans |= (static_cast<uint64_t>(exp) << (significand_bits));
    ans |= significand;

    return ans;
}

long double unpack754(uint64_t encoded_f, uint32_t bits, uint32_t exp_bits)
{
    if (encoded_f == 0)
        return 0;
    uint64_t bias = (1 << (exp_bits - 1)) - 1;
    uint64_t significand_bits = bits - exp_bits - 1;
    uint64_t sign = ((1ULL << (bits - 1)) & encoded_f) > 0;
    uint64_t significand = (encoded_f & ((1ULL << significand_bits) - 1));
    int64_t exp = ((encoded_f >> significand_bits) & ((1 << exp_bits) - 1));
    exp -= bias;

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
