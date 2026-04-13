#include "server_tcp.h"
#include "tcp_helpers.h"

void ServerTCP::listen_socket()
{
    log("Initiating listen...\n");
    if (listen(sock_desc, BACKLOG) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
    log("Server listening...\n");
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

    auto [msg_len, msg_type] = strip_headers(header);

    std::vector<char> payload(msg_len);
    if (recv(fd, payload.data(), msg_len, MSG_WAITALL) != msg_len)
        return;

    std::string buf(payload.data(), msg_len);
    size_t offset = 0;

    if (msg_type == MessageType::SUBMIT_ORDER)
    {
        SubmitOrderPayload order_payload{};
        unpack(buf, offset, order_payload);

        log("{}, {}, {}, {}", order_payload.user_id, static_cast<int>(order_payload.side), order_payload.price, order_payload.quantity);

        auto res = book.process_order(order_payload.side, order_payload.quantity, order_payload.price, order_payload.user_id);
        std::string response;
        if (std::holds_alternative<Order>(res))
        {
            construct_message<MessageType::ORDER_ACK>(response, std::get<Order>(res).id);
        }
        else if (std::holds_alternative<std::vector<Match>>(res))
        {
            construct_message<MessageType::MATCH>(response, std::get<std::vector<Match>>(res));
        }
        else
        {
            throw std::runtime_error("Order submit return type invalid");
        }

        tcp_send(fd, response);
    }
    else if (msg_type == MessageType::GET_ORDERS)
    {
        uint32_t customer_id;
        unpack(buf, offset, customer_id);

        std::vector<Order> orders = book.get_orders(customer_id);

        std::string response;
        construct_message<MessageType::ORDERS_LIST>(response, orders);
        tcp_send(fd, response);
    }
}

void ServerTCP::use_poll()
{
    pfds.push_back({sock_desc, POLLIN, 0});

    while (true)
    {
        int events = poll(&pfds[0], pfds.size(), -1);
        if (events == -1)
        {
            throw std::runtime_error("Poll failed with -1");
        }

        if (pfds[0].revents & POLLIN)
        {
            accept_socket();
        }

        for (int i = 1; i < pfds.size(); i++)
        {
            if ((pfds[i].revents & POLLHUP) || (pfds[i].revents & POLLERR))
            {
                if (pfds[i].revents & POLLHUP)
                    log("Client hung up, cleaning up fd...");
                else
                    log("Error on poll, cleaning up fd...");

                remove_fd(i);
            }
            else if (pfds[i].revents & POLLIN)
            {
                char peek;
                ssize_t n = recv(pfds[i].fd, &peek, 1, MSG_PEEK);
                if (n <= 0)
                    remove_fd(i);
                else
                    process_request(pfds[i].fd);
            }
        }
    }
}
