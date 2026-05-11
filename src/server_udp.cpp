#include "server_udp.h"

#include <stdexcept>

void ServerUDP::start_server()
{
    char buf[2];
    buf[1] = '\0';

    if (recvfrom(sock_desc, buf, sizeof(buf) - 1, 0, servinfo->ai_addr, &servinfo->ai_addrlen) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
}

void ServerUDP::use_poll() {}
