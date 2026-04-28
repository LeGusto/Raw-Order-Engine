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
#include "latency.h"
#include <csignal>

struct ClientBuffer
{
    char data[MAX_REQUEST_SIZE + 5]; // +5 for the header (4 length + 1 type)
    size_t received = 0;

    bool has_header() const { return received >= 5; }

    uint32_t msg_len() const
    {
        uint32_t len;
        std::memcpy(&len, &data, 4);
        return ntohl(len);
    }

    bool is_complete() const
    {
        return has_header() && received >= 5U + msg_len();
    }

    void reset() { received = 0; }
};

class Server
{
protected:
    addrinfo *servinfo = nullptr;
    addrinfo *servinfo_head = nullptr; // for freeing linked list
    int32_t sock_desc = -1;
    int32_t reuse_addr = 1;   // skip TIME_WAIT for closed ports, doesn't wait for leftover packets
    pollfd pfds[BACKLOG + 1]; // +1 for listener
    int pfd_count = 0;
    ClientBuffer client_buffers[BACKLOG + 1];
    OrderBook book;
    LatencyTracker tracker{LOG_ENTRIES};

    inline static volatile sig_atomic_t stop;
    static void handle_stop(int) { stop = 1; }

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