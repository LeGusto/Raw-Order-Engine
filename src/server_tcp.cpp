#include "server_tcp.h"
#include "tcp_helpers.h"
#include "chrono"
#include "enum_name.h"

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
    if (pfd_count >= BACKLOG + 1)
    {
        log("Server full, rejecting connection");
        close(new_fd);
        return;
    }

    pfds[pfd_count] = {new_fd, POLLIN, 0};
    client_buffers[pfd_count].reset();
    pfd_count++;
    log("Accepted connection:\n");
    print_sockaddr(reinterpret_cast<sockaddr *>(&sa), sa_len);
}

void ServerTCP::start_server()
{
    listen_socket();
    use_poll();
}

void ServerTCP::process_request(int i)
{
    int fd = pfds[i].fd;
    auto &buffer = client_buffers[i];
    auto [msg_len, msg_type] = strip_headers(buffer.data);

    if (msg_len > MAX_REQUEST_SIZE)
    {
        std::string response;
        construct_message<MessageType::REJECT>(response, RejectReason::PAYLOAD_TOO_LARGE);
        tcp_send(fd, response);
        buffer.reset();
        return;
    };

    size_t offset = 5; // skip header (4 length + 1 type)
    std::string response;

    auto t1 = std::chrono::steady_clock::now();

    if (msg_type == MessageType::SUBMIT_ORDER)
    {
        SubmitOrderPayload order_payload{};
        unpack(buffer.data, offset, order_payload);

        log("{}, {}, {}, {}", order_payload.user_id, static_cast<int>(order_payload.side), order_payload.price, order_payload.quantity);

        auto res = book.process_order(order_payload.side, order_payload.quantity, order_payload.price, order_payload.user_id);
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
    }
    else if (msg_type == MessageType::GET_ORDERS)
    {
        uint32_t customer_id;
        unpack(buffer.data, offset, customer_id);

        std::vector<Order> orders = book.get_orders(customer_id);

        construct_message<MessageType::ORDERS_LIST>(response, orders);
    }
    else if (msg_type == MessageType::CANCEL_ORDER)
    {
        uint32_t order_id;
        unpack(buffer.data, offset, order_id);

        auto res = book.cancel_order(order_id);

        if (res.has_value())
        {
            construct_message<MessageType::CANCEL_ACK>(response, order_id);
        }
        else
        {
            construct_message<MessageType::REJECT>(response, RejectReason::ORDER_NOT_FOUND);
        }
    }
    else
    {
        construct_message<MessageType::REJECT>(response, RejectReason::INVALID_MESSAGE);
    }

    auto t2 = std::chrono::steady_clock::now();

    uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    tracker.insert_entry(enum_name(msg_type), ns);

    tcp_send(fd, response);
    buffer.reset();
}

void ServerTCP::use_poll()
{
    pfds[0] = {sock_desc, POLLIN, 0};
    pfd_count = 1;

    while (!stop)
    {
        int events = poll(pfds, pfd_count, -1);
        if (events == -1)
        {
            if (errno == EINTR) // exit signal
                continue;
            throw std::runtime_error("Poll failed with -1");
        }

        try
        {
            if (pfds[0].revents & POLLIN)
            {
                if (pfd_count < BACKLOG + 1)
                {
                    accept_socket();
                }
            }
        }
        catch (const std::exception &e)
        {
            log("Failed to accept socket");
            log("Error: {}", e.what());
        }

        for (int i = 1; i < pfd_count; i++)
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
                try
                {
                    auto &buf = client_buffers[i];
                    ssize_t n = recv(pfds[i].fd, buf.data + buf.received, sizeof(buf.data) - buf.received, 0);
                    if (n <= 0)
                    {
                        remove_fd(i);
                    }
                    else
                    {
                        buf.received += n;
                        if (buf.is_complete())
                        {
                            process_request(i);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    log("Error processing client, removing...");
                    log("Error: {}", e.what());
                    remove_fd(i);
                }
            }
        }
    }
}
