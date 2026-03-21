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

    void log(std::string msg)
    {
        if constexpr (DEBUG)
        {
            std::cout << "[server] " << msg;
        }
    }

    void print_listen_sockaddr()
    {
        char host[200];
        char port[200];

        getnameinfo((sockaddr *)&listen_sockaddr, listen_sockaddr_addrlen, host, sizeof(host), port, sizeof(port), 0);

        std::cout << "Connected to host: " << host << "\n";
        std::cout << "Connected to port: " << port << "\n";
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

        std::print("[server] Address: {}\nPort: {}\nFamily: {}\nSocket type: {} \n\n", addr, port, family, socket_type);
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

        std::cout << "[server] All adresses: \n";

        for (auto &v : ips)
        {
            std::cout << v << "\n";
        }

        std::cout << "\n";
    }

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

    void listen_socket()
    {
        log("Initiating listen...\n");
        if (listen(sock_desc, BACKLOG) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        log("Server listening...\n");
    }

    void accept_socket()
    {
        log("Waiting for connection...\n");
        if ((listen_sock_desc = accept(sock_desc, reinterpret_cast<sockaddr *>(&listen_sockaddr), &listen_sockaddr_addrlen)) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        log("Accepted connection:\n");
        print_listen_sockaddr();
    }

    void send_msg(const char *msg)
    {
        size_t bytes_needed = strlen(msg);
        size_t bytes_sent = 0;

        log("Sending message...\n");
        while (bytes_needed > 0)
        {
            std::cout << "[server] Bytes left: " << bytes_needed << "\n";
            if ((bytes_sent = send(listen_sock_desc, msg, bytes_needed, 0)) == -1)
            {
                throw std::runtime_error(strerror(errno));
            }
            bytes_needed -= bytes_sent;
            msg += bytes_sent;
        }

        log("Message delivered\n");
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
    Server server{};
    server.get_socket();
    server.bind_socket();
    server.listen_socket();
    server.accept_socket();
    std::string msg = "hello";
    for (int i = 0; i < 29; i++)
    {
        msg += msg;
    }
    server.send_msg(&msg[0]);
}