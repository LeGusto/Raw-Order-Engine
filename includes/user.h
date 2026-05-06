#pragma once

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
#include <vector>
#include <print>
#include <thread>
#include <atomic>
#include <order_book.h>
#include <protocol.h>
#include "serializer.h"

class User
{
private:
    inline static std::atomic<uint32_t> user_count{0};
    addrinfo *servinfo_head = nullptr;
    addrinfo *servinfo = nullptr;
    int fd = -1;
    uint32_t user_id = 0;
    std::vector<uint32_t> order_ids;

    std::pair<MessageType, std::string> recv_response();

public:
    User();

    void log(std::string s);

    void get_addrinfo();

    void get_socket();

    void connect_socket();

    void use_server();

    void submit_order(Side side, uint32_t quantity, uint32_t price);
    void cancel_order(uint32_t order_id);
    void get_orders();

    ~User();
};