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
#include <array>
#include "order_book.h"
#include "protocol.h"
#include "serializer.h"

struct ClientBuffer
{
    char data[MAX_REQUEST_SIZE + 3]; // +3 for the header
    size_t received = 0;

    void append(const char *src, size_t len)
    {
        if (received + len > MAX_REQUEST_SIZE)
        {
            throw std::runtime_error("Client buffer append exceeds max capacity");
        }
        std::memcpy(&data + received, src, len);
        received += len;
    }

    bool has_header() const { return received >= 3; }

    uint16_t msg_len() const
    {
        uint16_t len;
        std::memcpy(&len, &data, 2);
        return ntohs(len);
    }

    bool is_complete() const
    {
        return has_header() && received >= 3 + msg_len();
    }

    void reset() { received = 0; }
};

class Server
{
protected:
    addrinfo *servinfo = nullptr;
    addrinfo *servinfo_head = nullptr; // for freeing linked list
    int32_t sock_desc = -1;
    int32_t reuse_addr = 1; // skip TIME_WAIT for closed ports, doesn't wait for leftover packets
    std::vector<pollfd> pfds;
    std::vector<ClientBuffer> client_buffers;
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

    void remove_fd(int &i);

private:
    void get_socket();

    void bind_socket();

public:
    Server();

    void print_sockaddr(sockaddr *sa, socklen_t len);

    void print_addrinfo();

    virtual void start_server() = 0;

    virtual void use_poll() = 0;

    virtual ~Server();
};