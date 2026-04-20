#pragma once

#include "server_base.h"
#include <vector>
#include <cstdint>
#include <csignal>

class ServerTCP : public Server
{

private:
    std::vector<uint64_t> latencies_ns;
    static volatile sig_atomic_t should_exit;

    static void signal_handler(int);

    void dump_latencies();

    void listen_socket();

    void accept_socket();

    void process_request(int fd);

    void start_server() override;

    void use_poll() override;
};