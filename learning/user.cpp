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

int main()
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *servinfo;
    int status = getaddrinfo(HOST, PORT, &hints, &servinfo);
    if (status != 0)
    {
        throw std::runtime_error(gai_strerror(status));
    }

    int socket_desc;
    while ((socket_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        if (servinfo->ai_next == nullptr)
        {
            throw std::runtime_error("All address nodes invalid");
        }
        servinfo = servinfo->ai_next;
    }

    if (connect(socket_desc, servinfo->ai_addr, servinfo->ai_addrlen))
    {
        throw std::runtime_error(strerror(errno));
    }

    char buf[20]{};
    buf[19] = '\0';

    int bytes_received = 0;
    while ((bytes_received = recv(socket_desc, &buf, sizeof(buf) - 1, 0)) != 0)
    {
        if (bytes_received == -1)
        {
            throw std::runtime_error(strerror(errno));
        }

        // std::cout << buf;
    };

    freeaddrinfo(servinfo);
    close(socket_desc);
}