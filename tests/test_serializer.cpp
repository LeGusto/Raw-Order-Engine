#include "order_book.h"
#include "serializer.h"
#include <cassert>
#include <cstring>

int passed = 0;
int failed = 0;

void check(bool cond, const char *name)
{
    if (cond)
    {
        printf("  PASS: %s\n", name);
        passed++;
    }
    else
    {
        printf("  FAIL: %s\n", name);
        failed++;
    }
}

void test_uint8()
{
    puts("== uint8_t ==");
    std::string buf;
    uint8_t val = 0xAB;
    pack(buf, val);
    check(buf.size() == 1, "size is 1 byte");

    size_t offset = 0;
    uint8_t out = 0;
    unpack(buf, offset, out);
    check(out == 0xAB, "value matches");
    check(offset == 1, "offset advanced");
}

void test_uint16()
{
    puts("== uint16_t ==");
    std::string buf;
    uint16_t val = 1234;
    pack(buf, val);
    check(buf.size() == 2, "size is 2 bytes");

    size_t offset = 0;
    uint16_t out = 0;
    unpack(buf, offset, out);
    check(out == 1234, "value matches");
    check(offset == 2, "offset advanced");
}

void test_uint32()
{
    puts("== uint32_t ==");
    std::string buf;
    uint32_t val = 100000;
    pack(buf, val);
    check(buf.size() == 4, "size is 4 bytes");

    size_t offset = 0;
    uint32_t out = 0;
    unpack(buf, offset, out);
    check(out == 100000, "value matches");
    check(offset == 4, "offset advanced");
}

void test_uint64()
{
    puts("== uint64_t ==");
    std::string buf;
    uint64_t val = 123456789012345ULL;
    pack(buf, val);
    check(buf.size() == 8, "size is 8 bytes");

    size_t offset = 0;
    uint64_t out = 0;
    unpack(buf, offset, out);
    check(out == 123456789012345ULL, "value matches");
    check(offset == 8, "offset advanced");
}

void test_int8()
{
    puts("== int8_t ==");
    std::string buf;
    int8_t val = -42;
    pack(buf, val);
    check(buf.size() == 1, "size is 1 byte");

    size_t offset = 0;
    int8_t out = 0;
    unpack(buf, offset, out);
    check(out == -42, "value matches");
}

void test_int32()
{
    puts("== int32_t ==");
    std::string buf;
    int32_t val = -99999;
    pack(buf, val);
    check(buf.size() == 4, "size is 4 bytes");

    size_t offset = 0;
    int32_t out = 0;
    unpack(buf, offset, out);
    check(out == -99999, "value matches");
}

void test_enum()
{
    puts("== enum Side ==");
    std::string buf;
    Side val = Side::BID;
    pack(buf, val);

    size_t offset = 0;
    Side out = Side::ASK;
    unpack(buf, offset, out);
    check(out == Side::BID, "value matches");
}

void test_order()
{
    puts("== Order struct ==");
    Order order(Side::BID, 50, 200, 7);
    std::string buf;
    pack(buf, order);

    size_t offset = 0;
    Order out(Side::ASK, 0, 0, 0);
    unpack(buf, offset, out);

    check(out.id == order.id, "id matches");
    check(out.quantity == 50, "quantity matches");
    check(out.price == 200, "price matches");
    check(out.customerID == 7, "customerID matches");
    check(out.side == Side::BID, "side matches");
}

void test_multiple_fields()
{
    puts("== multiple values in one buffer ==");
    std::string buf;
    uint32_t a = 42;
    uint16_t b = 1000;
    uint8_t c = 0xFF;
    pack(buf, a);
    pack(buf, b);
    pack(buf, c);
    check(buf.size() == 7, "total size is 7 bytes");

    size_t offset = 0;
    uint32_t out_a = 0;
    uint16_t out_b = 0;
    uint8_t out_c = 0;
    unpack(buf, offset, out_a);
    unpack(buf, offset, out_b);
    unpack(buf, offset, out_c);
    check(out_a == 42, "first value matches");
    check(out_b == 1000, "second value matches");
    check(out_c == 0xFF, "third value matches");
    check(offset == 7, "offset advanced correctly");
}

void test_float()
{
    puts("== float ==");
    std::string buf;
    float val = 3.14f;
    pack(buf, val);

    size_t offset = 0;
    float out = 0;
    unpack(buf, offset, out);
    check(std::abs(out - 3.14f) < 0.001f, "value matches");
}

void test_double()
{
    puts("== double ==");
    std::string buf;
    double val = -123.456;
    pack(buf, val);

    size_t offset = 0;
    double out = 0;
    unpack(buf, offset, out);
    check(std::abs(out - (-123.456)) < 0.001, "value matches");
}

void test_float_zero()
{
    puts("== float zero ==");
    std::string buf;
    float val = 0.0f;
    pack(buf, val);

    size_t offset = 0;
    float out = 1.0f;
    unpack(buf, offset, out);
    check(out == 0.0f, "zero matches");
}

void test_negative_float()
{
    puts("== negative float ==");
    std::string buf;
    float val = -42.5f;
    pack(buf, val);

    size_t offset = 0;
    float out = 0;
    unpack(buf, offset, out);
    check(std::abs(out - (-42.5f)) < 0.001f, "value matches");
}

int main()
{
    test_uint8();
    test_uint16();
    test_uint32();
    test_uint64();
    test_int8();
    test_int32();
    test_enum();
    test_order();
    test_multiple_fields();
    test_float();
    test_double();
    test_float_zero();
    test_negative_float();

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
