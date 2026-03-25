#pragma once

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
#include <fcntl.h>
#include "config.h"

class Server
{
protected:
    addrinfo *servinfo = nullptr;
    addrinfo *servinfo_head = nullptr; // for freeing linked list
    int sock_desc = -1;
    int reuse_port = 1; // skip TIME_WAIT for closed ports, doesn't wait for leftover packets
    int listen_sock_desc = -1;
    socklen_t listen_sockaddr_addrlen = sizeof(sockaddr_storage);
    sockaddr_storage listen_sockaddr; // sockaddr type agnostic storage

    void setup_addrinfo()
    {
        addrinfo hints;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AI_FAMILY;
        hints.ai_socktype = AI_SOCKTYPE;
        hints.ai_flags = AI_FLAGS;

        int status = getaddrinfo(NULL, PORT, &hints, &servinfo);

        if (status != 0)
        {
            throw std::runtime_error(gai_strerror(status));
        }

        servinfo_head = servinfo;
    }

    void log(std::string msg)
    {
        if constexpr (DEBUG)
        {
            std::cout << "[server] " << msg;
        }
    }

private:
    void get_socket()
    {
        if (servinfo == nullptr)
        {
            throw std::runtime_error("No address info available");
        }

        sock_desc = -1;

        log("Creating socket...\n");
        print_addrinfo();

        while ((sock_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
        {
            if (servinfo->ai_next == nullptr)
            {
                throw std::runtime_error("All address nodes invalid");
            }

            servinfo = servinfo->ai_next;
            log("Failed to get socket, trying different address\n");
        }

        setsockopt(sock_desc, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
        if (!BLOCKING)
        {
            fcntl(sock_desc, F_SETFL, O_NONBLOCK);
        }
        log("Socket created\n");
        // setsockopt(sock_desc, SOL_SOCKET, SO_SNDBUF, &SND_SIZE, sizeof(SND_SIZE));
    }

    void bind_socket()
    {
        log("Binding socket...\n");
        if (bind(sock_desc, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        log("Socket bound\n");
    }

public:
    Server()
    {
        setup_addrinfo();
        get_socket();
        bind_socket();
    };

    void print_listen_sockaddr()
    {
        char host[200];
        char port[200];

        getnameinfo((sockaddr *)&listen_sockaddr, listen_sockaddr_addrlen, host, sizeof(host), port, sizeof(port), 0);

        std::cout << "Connected to host: " << host << "\n";
        std::cout << "Connected to port: " << port << "\n";
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

        std::print("[server] Address: {}\nPort: {}\nFamily: {}\nSocket type: {} \n\n", addr, port, family, socket_type);
    }

    virtual void send_msg(const char *msg) = 0;

    virtual void get_connection() = 0;

    virtual ~Server()
    {
        if (servinfo != nullptr)
        {
            freeaddrinfo(servinfo_head);
        }
        if (sock_desc != -1)
        {
            close(sock_desc);
        }
        if (listen_sock_desc != -1)
        {
            close(listen_sock_desc);
        }
    }
};