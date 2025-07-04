#include "slow_client.hpp"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctime>
#include <bitset>
#include <iostream>

// Gera campo sttl_flags com 27 bits de TTL e 5 bits de flags
uint32_t encode_sttl_flags(uint32_t sttl, uint8_t flags) {
    return ((sttl & 0x07FFFFFF) << 5) | (flags & 0x1F);
}

void gerar_nil(uint8_t* uuid) 
/* Preenche o array UUID_SIZE com zeros */
{
    std::memset(uuid, 0, UUID_SIZE);
}

void print_uuid(const uint8_t* uuid) 
/* Exibe cada byte do UUID em hex */
{
    for (int i = 0; i < UUID_SIZE; ++i)
        printf("%02x", uuid[i]);
    printf("\n");
}

void print_binario(const void* data, size_t size) 
/* Mostra representação binária dos bytes em ambas endians */
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);

    std::cout << "Big Endian:\n";
    for (size_t i = 0; i < size; ++i) {
        std::bitset<8> bits(bytes[i]);
        std::cout << bits << " ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    if (size % 4 != 0) std::cout << "\n";

    std::cout << "Little Endian:\n";
    for (size_t i = 0; i < size; ++i) {
        std::bitset<8> bits(bytes[size - 1 - i]);
        std::cout << bits << " ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    if (size % 4 != 0) std::cout << "\n";
}

void print_flags(uint8_t flags) 
/* Exibe flags em binário e lista os nomes ativos */
{
    std::cout << std::bitset<5>(flags) << " (";
    bool printed = false;

    if (flags & FLAG_CONNECT) { std::cout << "CONNECT"; printed = true; }
    if (flags & FLAG_REVIVE)  { if (printed) std::cout << ", "; std::cout << "REVIVE"; printed = true; }
    if (flags & FLAG_ACK)     { if (printed) std::cout << ", "; std::cout << "ACK"; printed = true; }
    if (flags & FLAG_ACCEPT)  { if (printed) std::cout << ", "; std::cout << "ACCEPT"; printed = true; }
    if (flags & FLAG_MB)      { if (printed) std::cout << ", "; std::cout << "MB"; printed = true; }
    std::cout << ")\n";
}

uint32_t reverse_bits_32(uint32_t n) 
/* Retorna o valor com bits invertidos */
{
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result <<= 1;
        result |= (n & 1);
        n >>= 1;
    }
    return result;
}

void print_packet_info(const SlowPacket& pkt, ssize_t received_size, int type, int reenviado = 0)
/* Exibe no console todos os campos do pacote, indicando tipo (envio/recebimento/reenvio) */
{
    // std::cout << "TTL (27 bits): " << std::bitset<32>(pkt.sttl_flags) << std::endl;
    uint32_t ttl = (pkt.sttl_flags >> 5) & 0x07FFFFFF;
    uint32_t flags = pkt.sttl_flags & 0x00FFFFFF;

    if (type == 1) {
        std::cout << "\n-------- Pacote Recebido --------\n";
    } else if (type ==2){
        if (reenviado > 0){
            std::cout << "\n-------- Pacote Reenviado (" + std::to_string(reenviado) + "x) --------\n";
        } else{
            std::cout << "\n-------- Pacote Reenviado --------\n";
        }
    } else {
        std::cout << "\n-------- Pacote Enviado --------\n";
    }
    std::cout << "SID: ";
    print_uuid(pkt.sid);
    std::cout << "Flags: ";
    print_flags(flags);
    std::cout << "STTL: " << ttl << " s\n";
    std::cout << "SEQNUM: " << pkt.seqnum << "\n";
    std::cout << "ACKNUM: " << pkt.acknum << "\n";
    std::cout << "WINDOW: " << pkt.window << "\n";
    std::cout << "FID: " << static_cast<int>(pkt.fid)
              << " FO: " << static_cast<int>(pkt.fo) << "\n";

    size_t data_len = received_size > SLOW_HEADER_SIZE
                      ? received_size - SLOW_HEADER_SIZE
                      : 0;

    if (data_len > 0) {
        std::cout << "DATA (" << data_len << " bytes): ";
        for (size_t i = 0; i < data_len; ++i)
            std::cout << pkt.data[i];
        std::cout << "\n";
    }

    std::cout << "---------------------------------\n\n";
}
