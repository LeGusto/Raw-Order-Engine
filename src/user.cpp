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
#include <user.h>
#include <random>

User::User() : user_id(user_count++) {};

void User::log(std::string s)
{
    std::print("[user {}]: {}\n", user_id, s);
}

void User::get_addrinfo()
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AI_FAMILY;
    hints.ai_socktype = AI_SOCKTYPE;

    int status = getaddrinfo("localhost", PORT, &hints, &servinfo);
    if (status != 0)
    {
        throw std::runtime_error(gai_strerror(status));
    }
    servinfo_head = servinfo;
}

void User::get_socket()
{
    if ((fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
}

void User::connect_socket()
{
    while (connect(fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        if (servinfo->ai_next == nullptr)
        {
            throw std::runtime_error(strerror(errno));
        }
        servinfo = servinfo->ai_next;
    }
}

void User::submit_order(Side side, uint32_t quantity, uint32_t price)
{
    std::string payload;
    pack(payload, user_id);
    pack(payload, side);
    pack(payload, price);
    pack(payload, quantity);

    std::string msg;
    uint16_t len = htons(payload.size());
    msg.append(reinterpret_cast<const char *>(&len), 2);
    msg.push_back(static_cast<char>(MessageType::SUBMIT_ORDER));
    msg.append(payload);

    send(fd, msg.data(), msg.size(), 0);
}

uint16_t User::strip_msg_len(const char *&msg)
{
    uint16_t res;
    memcpy(&res, msg, 2);
    msg = msg + 2;

    return ntohs(res);
}

MessageType User::strip_msg_type(const char *&msg)
{
    MessageType res;
    memcpy(&res, msg, 1);
    msg++;

    return res;
}

void User::get_orders()
{
    std::string payload;
    pack(payload, user_id);

    std::string msg;
    uint16_t len = htons(payload.size());
    msg.append(reinterpret_cast<const char *>(&len), 2);
    msg.push_back(static_cast<char>(MessageType::GET_ORDERS));
    msg.append(payload);

    send(fd, msg.data(), msg.size(), 0);

    char header[3];
    if (recv(fd, header, 3, MSG_WAITALL) != 3)
    {
        std::cout << "wha\n";
        return;
    }

    const char *ptr = header;
    uint16_t msg_len = strip_msg_len(ptr);
    MessageType msg_type = strip_msg_type(ptr);

    if (msg_type == MessageType::ORDERS_LIST)
    {
        char response[MAX_REQUEST_SIZE];
        recv(fd, response, msg_len, MSG_WAITALL);
        std::string buf(response, msg_len);

        std::vector<Order> orders;

        size_t offset = 0;

        unpack(buf, offset, orders);

        for (auto &v : orders)
        {
            v.print();
        }
    }
}

void User::get_data()
{

    char buf[20];
    memset(&buf, '\0', sizeof(buf));
    send(fd, buf, 1, 0);

    int32_t len = 0;
    recv(fd, &len, sizeof(len), MSG_WAITALL); // waitall in case partial message gets delivered
    len = ntohl(len);
    while (len > 0)
    {
        int gotten = recv(fd, &buf, sizeof(buf) - 1, 0);
        len -= gotten;
        log(buf);
        memset(&buf, '\0', sizeof(buf));
    }
}

void User::use_server()
{
    get_addrinfo();
    get_socket();
    connect_socket();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> price_dist(50, 200);
    std::uniform_int_distribution<uint32_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);

    for (int i = 0; i < 5; i++)
    {
        Side side = static_cast<Side>(side_dist(gen));
        submit_order(side, qty_dist(gen), price_dist(gen));
    }

    get_orders();
}

User::~User()
{
    if (servinfo_head != nullptr)
        freeaddrinfo(servinfo_head);
    if (fd != -1)
        close(fd);
}
