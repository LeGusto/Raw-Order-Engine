#include <iomanip>
#include <iostream>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void print_bits(int n)
{
    std::cout << n << ": ";
    std::vector<int> b;
    while (n)
    {
        b.push_back(n & 1);
        n >>= 1;
    }

    while (b.size() < 32)
        b.push_back(0);

    std::reverse(b.begin(), b.end());

    for (auto &v : b)
        std::cout << v;
    std::cout << "\n";
}

void print_power(int n)
{
    int power = (int)std::log2(n);
    std::cout << "2^" << power << "\n";
}

void test_ntox()
{
    int a;
    std::cin >> a;

    print_power(a);
    a = ntohl(a);
    print_power(a);
}

void test_inet_pton()
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, "0.0.1.1", &sa.sin_addr); // -1 on error, 0 on wrong addr format

    char res[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr.s_addr, &res[0], INET_ADDRSTRLEN); // convert to presentation
    std::cout << ntohl(sa.sin_addr.s_addr) << ": " << res << " " << "\n";

    struct sockaddr_in6 sa6;
    inet_pton(AF_INET6, "0000:0000:0000:0000::0000:0000:0001", &sa6.sin6_addr);

    char res6[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sa6.sin6_addr, &res6[0], INET6_ADDRSTRLEN);

    std::cout << res6 << "\n";
}

void test_getaddrinfo()
{
    addrinfo *ai;
    if (getaddrinfo("google.com", "80", nullptr, &ai) != 0)
    {
        std::cout << "Failed\n";
    }

    if (ai->ai_family == AF_INET)
    {
        char res[INET_ADDRSTRLEN];
        sockaddr_in *sa = reinterpret_cast<sockaddr_in *>(ai->ai_addr);

        inet_ntop(AF_INET, &sa->sin_addr.s_addr, &res[0], INET_ADDRSTRLEN);
        std::cout << res << "\n";
    }
}

void testing()
{
    const char msg[] = {'c', 'a', 'n', '\0'};
    const char msg2[] = {};
    std::cout << sizeof(&msg) << std::endl;
    std::cout << sizeof(&msg2) << std::endl;
    ;
}

int main()
{
    // test_getaddrinfo();
    testing();
}