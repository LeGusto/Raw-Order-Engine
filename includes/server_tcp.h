#pragma once

#include "server_base.h"

class ServerTCP : public Server
{

private:
    void listen_socket();

    void accept_socket();

    void process_request(int fd);

    void start_server() override;

    void use_poll() override;
};
