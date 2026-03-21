#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <iostream>
#include <print>
#include <cstddef>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include "config.h"

class Server
{
private:
    addrinfo *servinfo = nullptr;
    addrinfo *servinfo_head = nullptr; // for freeing linked list
    int sock_desc = -1;
    int reuse_port = 1; // skip TIME_WAIT for closed ports, doesn't wait for leftover packets
    int listen_sock_desc = -1;
    socklen_t listen_sockaddr_addrlen = sizeof(sockaddr_storage);
    sockaddr_storage listen_sockaddr; // sockaddr type agnostic storage

public:
    Server(const Server &) = delete;            // no copy Server(s)
    Server &operator=(const Server &) = delete; // no copy Server b = s

    Server()
    {
        read_TCP_v4();
    }

    void read_TCP_v4()
    {
        addrinfo hints;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE; // fill in with own IP

        int status = getaddrinfo(NULL, PORT, &hints, &servinfo);

        if (status != 0)
        {
            throw std::runtime_error(gai_strerror(status));
        }

        servinfo_head = servinfo;
    }

    void print_addrinfo()
    {
        if (servinfo == nullptr)
        {
            throw std::runtime_error("No address info available");
        }

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

    void print_all_ips()
    {
        if (servinfo == nullptr)
        {
            throw std::runtime_error("No address info available");
        }

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

    void get_socket()
    {
        if (servinfo == nullptr)
        {
            throw std::runtime_error("No address info available");
        }

        sock_desc = -1;

        while ((sock_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
        {
            if (servinfo->ai_next == nullptr)
            {
                throw std::runtime_error("All address nodes invalid");
            }

            servinfo = servinfo->ai_next;
        }

        setsockopt(sock_desc, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
    }

    void bind_socket()
    {
        if (bind(sock_desc, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void listen_socket()
    {
        if (listen(sock_desc, BACKLOG) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void accept_socket()
    {
        if ((listen_sock_desc = accept(sock_desc, reinterpret_cast<sockaddr *>(&listen_sockaddr), &listen_sockaddr_addrlen)) == -1)
        {
            std::runtime_error(strerror(errno));
        }
    }

    void send_msg(const char *msg)
    {
        int bytes_sent = send(listen_sock_desc, msg, strlen(msg), 0);
    }

    ~Server()
    {
        if (servinfo != nullptr)
        {
            freeaddrinfo(servinfo_head);
        }
        if (sock_desc != -1)
        {
            close(sock_desc);
        }
    }
};

int main()
{
    Server server{}; // anything below 1024 is reserved, valid up to 65535
    server.print_addrinfo();
    server.print_all_ips();
    server.get_socket();
    server.bind_socket();
    server.listen_socket();
    server.accept_socket();
    server.send_msg("Hello");
}