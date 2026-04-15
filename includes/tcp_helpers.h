#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <stdexcept>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "protocol.h"

inline std::pair<uint16_t, MessageType> strip_headers(const char *header)
{
    uint16_t len;
    memcpy(&len, header, 2);

    MessageType type;
    memcpy(&type, header + 2, 1);

    return {ntohs(len), type};
}

inline void tcp_send(int fd, const std::string &msg)
{
    ssize_t total_sent = 0;
    ssize_t to_send = msg.size();

    while (total_sent < to_send)
    {
        ssize_t bytes_sent = send(fd, msg.data() + total_sent, to_send - total_sent, 0);
        if (bytes_sent == -1)
            throw std::runtime_error(strerror(errno));
        total_sent += bytes_sent;
    }
}

inline void tcp_recv(int fd, char *buf, size_t len)
{
    size_t received = 0;
    while (received < len)
    {
        ssize_t n = recv(fd, buf + received, len - received, 0);
        if (n <= 0)
        {
            throw std::runtime_error("Failed to recv");
        }
        received += n;
        len -= n;
    }
}