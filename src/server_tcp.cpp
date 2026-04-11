#include "server_tcp.h"

void ServerTCP::listen_socket()
{
    log("Initiating listen...\n");
    if (listen(sock_desc, BACKLOG) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
    log("Server listening...\n");
}

uint16_t ServerTCP::strip_msg_len(const char *&msg)
{
    uint16_t res;
    memcpy(&res, msg, 2);
    msg = msg + 2;

    return ntohs(res);
}

MessageType ServerTCP::strip_msg_type(const char *&msg)
{
    MessageType res;
    memcpy(&res, msg, 1);
    msg++;

    return res;
}

void ServerTCP::accept_socket()
{
    int32_t new_fd = -1;
    sockaddr_storage sa{};
    socklen_t sa_len = sizeof(sa);
    log("Waiting for connection...\n");
    if ((new_fd = accept(sock_desc, reinterpret_cast<sockaddr *>(&sa), &sa_len)) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
    pfds.push_back({new_fd, POLLIN, 0});
    log("Accepted connection:\n");
    print_sockaddr(reinterpret_cast<sockaddr *>(&sa), sa_len);
}

void ServerTCP::send_msg(const char *msg, int fd)
{
    ssize_t bytes_needed = strlen(msg);
    ssize_t bytes_sent = 0;

    uint32_t net_len = htonl(bytes_needed);

    if ((bytes_sent = send(fd, &net_len, sizeof(net_len), 0)) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }

    while (bytes_needed > 0)
    {
        log("Bytes left: {}", bytes_needed);
        if ((bytes_sent = send(fd, msg, bytes_needed, 0)) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        bytes_needed -= bytes_sent;
        msg += bytes_sent;
    }

    log("Sent\n");
}

void ServerTCP::start_server()
{
    listen_socket();
    use_poll();
}

void ServerTCP::process_request(int fd)
{
    char header[3];
    if (recv(fd, header, 3, MSG_WAITALL) != 3)
        return;

    const char *ptr = header;
    uint16_t msg_len = strip_msg_len(ptr);
    MessageType msg_type = strip_msg_type(ptr);

    char payload[MAX_REQUEST_SIZE];
    if (recv(fd, payload, msg_len, MSG_WAITALL) != msg_len)
        return;

    std::string buf(payload, msg_len);
    size_t offset = 0;

    if (msg_type == MessageType::SUBMIT_ORDER)
    {
        uint32_t customer_id;
        Side side;
        uint32_t price;
        uint32_t quantity;

        unpack(buf, offset, customer_id);
        unpack(buf, offset, side);
        unpack(buf, offset, price);
        unpack(buf, offset, quantity);

        log("{}, {}, {}, {}", customer_id, static_cast<int>(side), price, quantity);

        book.process_order(side, quantity, price, customer_id);
    }
    else if (msg_type == MessageType::GET_ORDERS)
    {
        log("PRINT");
        uint32_t customer_id;
        unpack(buf, offset, customer_id);

        std::string payload;

        std::vector<Order> orders = book.get_orders(customer_id);
        for (auto &v : orders)
        {
            v.print();
        }
        pack(payload, orders);

        std::string response;
        uint16_t payload_len = payload.size();
        payload_len = htons(payload_len);
        response.append(reinterpret_cast<const char *>(&payload_len), 2);
        response.push_back(static_cast<char>(MessageType::ORDERS_LIST));
        response += payload;
        send(fd, response.data(), response.size(), 0);
    }
}

void ServerTCP::use_poll()
{
    pfds.push_back({sock_desc, POLLIN, 0});

    while (true)
    {
        int events = poll(&pfds[0], pfds.size(), -1);

        if (pfds[0].revents & POLLIN)
        {
            accept_socket();
        }

        for (int i = 1; i < pfds.size(); i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                process_request(pfds[i].fd);
            }
        }
    }
}
