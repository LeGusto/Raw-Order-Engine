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
#include <poll.h>
#include "order_book.h"

class Server
{
protected:
    addrinfo *servinfo = nullptr;
    addrinfo *servinfo_head = nullptr; // for freeing linked list
    int32_t sock_desc = -1;
    int32_t reuse_port = 1; // skip TIME_WAIT for closed ports, doesn't wait for leftover packets
    std::vector<pollfd> pfds;
    OrderBook book;

    void setup_addrinfo();

    // evaluate parameter pack matches string format at compile time
    template <typename... Args>
    void log(std::format_string<Args...> fmt, Args &&...args)
    {
        if constexpr (DEBUG)
        {
            std::print("[server] ");
            std::print(fmt, std::forward<Args>(args)...);
            std::print("\n");
        }
    }

private:
    void get_socket();

    void bind_socket();

public:
    Server();

    void print_sockaddr(sockaddr *sa, socklen_t len);

    void print_addrinfo();

    virtual void send_msg(const char *msg, int fd) = 0;

    virtual void start_server() = 0;

    virtual void use_poll() = 0;

    virtual ~Server();
};