#ifndef SLOW_HPP
#define SLOW_HPP

#include <cstdint>
#include <string>
#include <netinet/in.h>

#define UUID_SIZE 16
#define MAX_DATA_SIZE 1440
#define SLOW_HEADER_SIZE 32
#define SERVER_PORT 7033

// Flags
#define FLAG_CONNECT 0x10
#define FLAG_REVIVE  0x08
#define FLAG_ACK     0x04
#define FLAG_ACCEPT  0x02
#define FLAG_MB      0x01

struct SlowPacket {
    uint8_t sid[UUID_SIZE];
    uint32_t sttl_flags;
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t window;
    uint8_t fid;
    uint8_t fo;
    uint8_t data[MAX_DATA_SIZE];
};

// Utilitários
uint32_t encode_sttl_flags(uint32_t sttl, uint8_t flags);
void gerar_nil(uint8_t* uuid);
void print_flags(uint8_t flags);
void print_uuid(const uint8_t* uuid);
void print_binario(const SlowPacket& pkt, ssize_t received_size, bool type);

// Classe do Peripheral
class SlowClient {
private:
    int sockfd;
    sockaddr_in server_addr;
    uint8_t session_id[UUID_SIZE];
    uint32_t session_ttl;
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t window_size;

public:
    explicit SlowClient(const std::string& server_ip);
    ~SlowClient();

    bool send_connect();
    bool receive_setup();
    bool send_data(const uint8_t* data, size_t length);
    bool send_ack();
    bool send_disconnect();
    bool receive_response();
};

#endif // SLOW_HPP
