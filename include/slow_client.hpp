#ifndef SLOW_HPP
#define SLOW_HPP

#include <cstdint>
#include <string>
#include <netinet/in.h>
#include "utils.hpp"

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

    uint16_t fid_counter = 0; 

public:
    explicit SlowClient(const std::string& server_ip);
    ~SlowClient();

    bool send_connect();
    bool send_data(const uint8_t* data, size_t length);
    bool send_fragmented_data(const uint8_t* data, size_t length);
    bool send_ack();
    bool send_disconnect();
    bool send_more_bits_data();
    bool send_zero_way();
    bool receive_response();
    bool receive_setup();
};

#endif // SLOW_HPP
