#include "server_base.h"

class ServerTCP : public Server
{

private:
    void listen_socket()
    {
        log("Initiating listen...\n");
        if (listen(sock_desc, BACKLOG) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        log("Server listening...\n");
    }

    void accept_socket()
    {

        log("Waiting for connection...\n");
        if ((listen_sock_desc = accept(sock_desc, reinterpret_cast<sockaddr *>(&listen_sockaddr), &listen_sockaddr_addrlen)) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        log("Accepted connection:\n");
        print_listen_sockaddr();
    }

public:
    void send_msg(const char *msg) override
    {
        ssize_t bytes_needed = strlen(msg);
        ssize_t bytes_sent = 0;

        while (bytes_needed > 0)
        {
            std::cout << "[server] Bytes left: " << bytes_needed << "\n";
            if ((bytes_sent = send(listen_sock_desc, msg, bytes_needed, 0)) == -1)
            {
                throw std::runtime_error(strerror(errno));
            }
            bytes_needed -= bytes_sent;
            msg += bytes_sent;
        }
    }

    void get_connection() override
    {
        listen_socket();
        accept_socket();
    }
};