#include "server_tcp.h"
#include "server_udp.h"
#include "config.h"
#include <memory>

int main()
{
    std::unique_ptr<Server> server;
    if (AI_SOCKTYPE == SOCK_STREAM)
    {
        server = std::make_unique<ServerTCP>();
    }
    else if (AI_SOCKTYPE == SOCK_DGRAM)
    {
        server = std::make_unique<ServerUDP>();
    }
    else
    {
        throw std::runtime_error("Unknown socket type");
    }
    server->get_connection();
    server->use_poll();
    // server->get_connection();
    // const char *buf = "Hello";
    // server->send_msg(buf);
}