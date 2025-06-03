#ifndef SLOW_HPP
#define SLOW_HPP

#include <cstdint>
#include <string>
#include <netinet/in.h>
#include "janela_envio.hpp"
#include "slow_packet.hpp"

#define SERVER_PORT 7033

// Flags
#define FLAG_CONNECT 0x10
#define FLAG_REVIVE  0x08
#define FLAG_ACK     0x04
#define FLAG_ACCEPT  0x02
#define FLAG_MB      0x01

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

    JanelaEnvio janela_envio;
    uint16_t fid_counter = 0; 

public:
    explicit SlowClient(const std::string& server_ip);
    ~SlowClient();

    // Exibe os pacotes pendentes armazenados na janela
    void debug_print_pacotes_pendentes() const;

    bool janela_tem_pacotes_pendentes() const;

    bool process_received_packet(SlowPacket& packet_out, ssize_t& received_bytes, bool flag_print);
    bool send_connect();
    bool reenviar_pacote(const SlowPacket& pkt);
    JanelaEnvio& get_janela_envio() { return janela_envio; }
    bool send_fragmented_data(const uint8_t* data, size_t length);
    bool receive_setup();
    bool send_data(const uint8_t* data, size_t length);
    bool send_ack();
    bool send_disconnect();
    bool receive_response();
};

#endif // SLOW_HPP
