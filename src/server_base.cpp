#include "server_base.h"

void Server::setup_addrinfo()
{

    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AI_FAMILY;
    hints.ai_socktype = AI_SOCKTYPE;
    hints.ai_flags = AI_FLAGS;

    int status = getaddrinfo(NULL, PORT, &hints, &servinfo);

    if (status != 0)
    {
        throw std::runtime_error(gai_strerror(status));
    }

    servinfo_head = servinfo;
}

void Server::get_socket()
{
    if (servinfo == nullptr)
    {
        throw std::runtime_error("No address info available");
    }

    sock_desc = -1;

    log("Creating socket...\n");
    print_addrinfo();

    while ((sock_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        if (servinfo->ai_next == nullptr)
        {
            throw std::runtime_error("All address nodes invalid");
        }

        servinfo = servinfo->ai_next;
        log("Failed to get socket, trying different address\n");
    }

    setsockopt(sock_desc, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
    if (!BLOCKING)
    {
        fcntl(sock_desc, F_SETFL, O_NONBLOCK);
    }
    log("Socket created\n");
    // setsockopt(sock_desc, SOL_SOCKET, SO_SNDBUF, &SND_SIZE, sizeof(SND_SIZE));
}

void Server::bind_socket()
{
    log("Binding socket...\n");
    if (bind(sock_desc, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
    log("Socket bound\n");
}

Server::Server()
{
    setup_addrinfo();
    get_socket();
    bind_socket();
};

void Server::print_sockaddr(sockaddr *sa, socklen_t len)
{
    char host[200];
    char port[200];

    if (getnameinfo(sa, len, host, sizeof(host), port, sizeof(port), 0) == -1)
    {
        throw std::runtime_error(strerror(errno));
    };

    log("Connected to host: {}", host);
    log("Connected to port: {}\n", port);
}

void Server::print_addrinfo()
{
    if (servinfo == nullptr)
    {
        throw std::runtime_error("No address info available");
    }

    std::string addr = "Unknown";
    short port = -1;
    std::string family = "Unknown";
    std::string socket_type = "Unknown";

    if (servinfo->ai_family == AF_UNSPEC)
        family = "Unspecified";
    else if (servinfo->ai_family == AF_INET)
    {
        family = "IPv4";
        addr.resize(INET_ADDRSTRLEN, ' ');
        sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(servinfo->ai_addr);

        inet_ntop(AF_INET, &ipv4->sin_addr, &addr[0], INET_ADDRSTRLEN);
        port = ntohs(ipv4->sin_port);
    }
    else if (servinfo->ai_family == AF_INET6)
    {
        family = "IPv6";
        addr.resize(INET6_ADDRSTRLEN, ' ');
        sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(servinfo->ai_addr);

        inet_ntop(AF_INET6, &ipv6->sin6_addr, &addr[0], INET6_ADDRSTRLEN);
        port = ntohs(ipv6->sin6_port);
    }

    if (servinfo->ai_socktype == SOCK_STREAM)
        socket_type = "TCP";
    else if (servinfo->ai_socktype == SOCK_DGRAM)
        socket_type = "UDP";

    log("Address: {}\nPort: {}\nFamily: {}\nSocket type: {} \n\n", addr, port, family, socket_type);
}

Server::~Server()
{
    if (servinfo != nullptr)
    {
        freeaddrinfo(servinfo_head);
    }
    if (sock_desc != -1)
    {
        close(sock_desc);
    }
    for (auto &v : pfds)
    {
        close(v.fd);
    }
}
