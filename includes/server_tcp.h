#include "server_base.h"
#include "protocol.h"
#include "serializer.h"

class ServerTCP : public Server
{

private:
    void listen_socket();

    void accept_socket();

    uint16_t strip_msg_len(const char *&msg);
    MessageType strip_msg_type(const char *&msg);

    void process_request(int fd);

public:
    void send_msg(const char *msg, int fd) override;

    void start_server() override;

    void use_poll() override;
};