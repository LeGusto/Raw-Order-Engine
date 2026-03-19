#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <iostream>
#include <print>
#include <cstddef>
#include <arpa/inet.h>
#include <vector>

class Server
{
public:
    static addrinfo *read_TCP_v4(const char name[])
    {
        addrinfo *servinfo;
        addrinfo hints;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        // hints.ai_flags = AI_PASSIVE;

        int status = getaddrinfo(name, "80", &hints, &servinfo);

        if (status != 0)
        {
            throw std::runtime_error(gai_strerror(status));
        }

        return servinfo;
    }

    static void print_addrinfo(addrinfo *servinfo)
    {
        std::string addr = "Unknown";
        short port = -1;
        std::string family = "Unknown";
        std::string socket_type = "Unknown";

        if (servinfo->ai_family == AF_UNSPEC)
            family = "Unspecified";
        else if (servinfo->ai_family == AF_INET)
        {
            family = "IPv4";
            addr.resize(INET_ADDRSTRLEN, ' ');
            sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(servinfo->ai_addr);

            inet_ntop(AF_INET, &ipv4->sin_addr, &addr[0], INET_ADDRSTRLEN);
            port = ntohs(ipv4->sin_port);
        }
        else if (servinfo->ai_family == AF_INET6)
        {
            family = "IPv6";
            addr.resize(INET6_ADDRSTRLEN, ' ');
            sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(servinfo->ai_addr);

            inet_ntop(AF_INET6, &ipv6->sin6_addr, &addr[0], INET6_ADDRSTRLEN);
            port = ntohs(ipv6->sin6_port);
        }

        if (servinfo->ai_socktype == SOCK_STREAM)
            socket_type = "TCP";
        else if (servinfo->ai_socktype == SOCK_DGRAM)
            socket_type = "UDP";

        std::print("Address: {}\n Port: {}\n Family: {}\n Socket type: {} \n\n", addr, port, family, socket_type);
    }

    static void print_all_ips(addrinfo *servinfo)
    {
        char res[INET6_ADDRSTRLEN];

        std::vector<std::string> ips;

        for (addrinfo *ai = servinfo; ai != NULL; ai = ai->ai_next)
        {
            if (ai->ai_family == AF_INET)
            {
                sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(ai->ai_addr);
                inet_ntop(AF_INET, &ipv4->sin_addr, &res[0], INET_ADDRSTRLEN);
                ips.push_back(res);
            }
            else if (ai->ai_family == AF_INET6)
            {
                sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(ai->ai_addr);
                inet_ntop(AF_INET6, &ipv6->sin6_addr, &res[0], INET6_ADDRSTRLEN);
                ips.push_back(res);
            }
        }

        std::cout << "All adresses: \n";

        for (auto &v : ips)
        {
            std::cout << v << "\n";
        }
    }
};

int main()
{
    addrinfo *servinfo = Server::read_TCP_v4("google.com");
    Server::print_addrinfo(servinfo);
    Server::print_all_ips(servinfo);
    freeaddrinfo(servinfo);
}