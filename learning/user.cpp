#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <cstddef>
#include "config.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

addrinfo *servinfo;
int socket_desc;

void get_udp()
{
    char buf[20]{};
    memset(buf, 0, sizeof(buf));
    buf[19] = '\0';

    struct sockaddr from;
    socklen_t fromlen = sizeof(from);

    sendto(socket_desc, buf, sizeof(buf), 0, servinfo->ai_addr, servinfo->ai_addrlen);
    int bytes_received = 0;
    while ((bytes_received = recvfrom(socket_desc, &buf, sizeof(buf) - 1, 0, &from, &fromlen)) != 0)
    {
        if (bytes_received == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        std::cout << buf;
    };
}

void get_tcp()
{
    if (connect(socket_desc, servinfo->ai_addr, servinfo->ai_addrlen))
    {
        throw std::runtime_error(strerror(errno));
    }

    char buf[20]{};
    memset(buf, 0, sizeof(buf));
    buf[19] = '\0';

    send(socket_desc, buf, 1, 0);

    int bytes_needed = 0;
    recv(socket_desc, &bytes_needed, sizeof(bytes_needed), 0);
    bytes_needed = ntohl(bytes_needed);

    int bytes_received = 0;
    while (bytes_needed > 0)
    {
        bytes_received = recv(socket_desc, &buf, sizeof(buf) - 1, 0);
        if (bytes_received == -1)
        {
            throw std::runtime_error(strerror(errno));
        }

        bytes_needed -= bytes_received;

        std::cout << bytes_needed << " " << buf << "\n";
    };
    std::cout << "Done\n";
}

int main()
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AI_FAMILY;
    hints.ai_socktype = AI_SOCKTYPE;

    int status = getaddrinfo(HOST, PORT, &hints, &servinfo);
    if (status != 0)
    {
        throw std::runtime_error(gai_strerror(status));
    }

    while ((socket_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        if (servinfo->ai_next == nullptr)
        {
            throw std::runtime_error("All address nodes invalid");
        }
        servinfo = servinfo->ai_next;
    }

    if (servinfo->ai_socktype == SOCK_STREAM)
        get_tcp();
    else if (servinfo->ai_socktype == SOCK_DGRAM)
        get_udp();

    freeaddrinfo(servinfo);
    close(socket_desc);
}