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
#include "tcp_helpers.h"

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
    SubmitOrderPayload payload{user_id, side, price, quantity};
    std::string msg;
    construct_message<MessageType::SUBMIT_ORDER>(msg, payload);
    tcp_send(fd, msg);

    char header[3];
    tcp_recv(fd, header, 3);

    auto [msg_len, msg_type] = strip_headers(header);

    std::vector<char> response(msg_len);
    tcp_recv(fd, response.data(), msg_len);

    std::string buf(response.data(), msg_len);
    size_t offset = 0;

    if (msg_type == MessageType::ORDER_ACK)
    {
        uint32_t order_id;
        unpack(buf.data(), offset, order_id);
        order_ids.push_back(order_id);
        log(std::format("Order acknowledged: id={}", order_id));
    }
    else if (msg_type == MessageType::MATCH)
    {
        std::vector<Match> matches;
        unpack(buf.data(), offset, matches);
        for (auto &m : matches)
        {
            log(std::format("Match: ask={}, bid={}, qty={}", m.ask_order.id, m.bid_order.id, m.quantity));
        }
    }
}

void User::cancel_order(uint32_t order_id)
{
    std::string msg;
    construct_message<MessageType::CANCEL_ORDER>(msg, order_id);
    tcp_send(fd, msg);

    char header[3];
    tcp_recv(fd, header, 3);

    auto [msg_len, msg_type] = strip_headers(header);

    std::vector<char> response(msg_len);
    tcp_recv(fd, response.data(), msg_len);

    std::string buf(response.data(), msg_len);
    size_t offset = 0;

    if (msg_type == MessageType::CANCEL_ACK)
    {
        uint32_t cancelled_id;
        unpack(buf.data(), offset, cancelled_id);
        log(std::format("Order cancelled: id={}", cancelled_id));
        std::erase(order_ids, cancelled_id);
    }
    else if (msg_type == MessageType::REJECT)
    {
        RejectReason reason;
        unpack(buf.data(), offset, reason);
        log(std::format("Cancel rejected: reason={}", static_cast<int>(reason)));
    }
}

void User::get_orders()
{
    std::string msg;
    construct_message<MessageType::GET_ORDERS>(msg, user_id);
    tcp_send(fd, msg);

    char header[3];
    tcp_recv(fd, header, 3);

    auto [msg_len, msg_type] = strip_headers(header);

    if (msg_type == MessageType::ORDERS_LIST)
    {
        std::vector<char> response(msg_len);
        tcp_recv(fd, response.data(), msg_len);
        std::string buf(response.data(), msg_len);

        std::vector<Order> orders;
        size_t offset = 0;
        unpack(buf.data(), offset, orders);

        for (auto &v : orders)
        {
            log(std::format("Order(id={}, side={}, qty={}, price={}, customer={})",
                            v.id, v.side == Side::ASK ? "ASK" : "BID", v.quantity, v.price, v.customerID));
        }
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
    std::uniform_int_distribution<int> cancel_chance(0, 3); // 25% chance to cancel

    for (int i = 0; i < USER_REQUESTS; i++)
    {
        Side side = static_cast<Side>(side_dist(gen));
        submit_order(side, qty_dist(gen), price_dist(gen));

        if (!order_ids.empty() && cancel_chance(gen) == 0)
        {
            std::uniform_int_distribution<int> idx_dist(0, order_ids.size() - 1);
            cancel_order(order_ids[idx_dist(gen)]);
        }
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
