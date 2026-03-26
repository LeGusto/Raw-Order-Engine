#include "server_base.h"

class ServerUDP : public Server
{
public:
    void send_msg(const char *msg, int fd) override
    {
        size_t bytes_needed = strlen(msg);
        ssize_t bytes_sent = 0;

        while (bytes_needed > 0)
        {
            std::cout << "[server] Bytes left: " << bytes_needed << "\n";
            if ((bytes_sent = sendto(sock_desc, msg, std::min(UDP_PACKET_SIZE, bytes_needed), 0, reinterpret_cast<sockaddr *>(&listen_sockaddr), listen_sockaddr_addrlen)) == -1)
            {
                throw std::runtime_error(strerror(errno));
            }
            bytes_needed -= bytes_sent;
            msg += bytes_sent;
        }

        if ((bytes_sent = sendto(sock_desc, msg, 0, 0, reinterpret_cast<sockaddr *>(&listen_sockaddr), listen_sockaddr_addrlen)) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void get_connection() override
    {
        char buf[2];
        buf[1] = '\0';

        if (recvfrom(sock_desc, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr *>(&listen_sockaddr), &listen_sockaddr_addrlen) == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
    }

    void use_poll() override {}
};