#include "slow_client.hpp"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctime>
#include <bitset>
#include <iostream>

uint32_t encode_sttl_flags(uint32_t sttl, uint8_t flags) {
    return ((sttl & 0x07FFFFFF) << 5) | (flags & 0x1F);
}

void gerar_nil(uint8_t* uuid) {
    std::memset(uuid, 0, UUID_SIZE);
}

void print_uuid(const uint8_t* uuid) {
    for (int i = 0; i < UUID_SIZE; ++i)
        printf("%02x", uuid[i]);
    printf("\n");
}

void print_binario(const void* data, size_t size) {
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

void print_flags(uint8_t flags) {
    std::cout << std::bitset<5>(flags) << " (";
    bool printed = false;

    if (flags & FLAG_CONNECT) { std::cout << "CONNECT"; printed = true; }
    if (flags & FLAG_REVIVE)  { if (printed) std::cout << ", "; std::cout << "REVIVE"; printed = true; }
    if (flags & FLAG_ACK)     { if (printed) std::cout << ", "; std::cout << "ACK"; printed = true; }
    if (flags & FLAG_ACCEPT)  { if (printed) std::cout << ", "; std::cout << "ACCEPT"; printed = true; }
    if (flags & FLAG_MB)      { if (printed) std::cout << ", "; std::cout << "MB"; printed = true; }
    std::cout << ")\n";
}

uint32_t reverse_bits_32(uint32_t n) {
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result <<= 1;
        result |= (n & 1);
        n >>= 1;
    }
    return result;
}

void print_packet_info(const SlowPacket& pkt, ssize_t received_size, bool type) {
    uint32_t ttl = (pkt.sttl_flags >> 5) & 0x07FFFFFF;
    uint32_t flags = pkt.sttl_flags & 0x00FFFFFF;

    if (type) {
        std::cout << "\n-------- Pacote Recebido --------\n";
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


SlowClient::SlowClient(const std::string& server_ip) {
    srand(time(nullptr));
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
        throw std::runtime_error("Erro ao criar socket");

    struct timeval tv = {5, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    gerar_nil(session_id);
    seqnum = 0;
    acknum = 0;
    window_size = 64;
}


SlowClient::~SlowClient() {
    close(sockfd);
}

bool SlowClient::send_connect() {
    SlowPacket pkt{};
    std::memset(&pkt, 0, sizeof(pkt));
    std::memcpy(pkt.sid, session_id, UUID_SIZE); // sid = UUID nil
    pkt.sttl_flags = encode_sttl_flags(0, FLAG_CONNECT);
    pkt.seqnum = 0;
    pkt.acknum = 0;
    pkt.window = 1024;
    pkt.fid = 0;
    pkt.fo = 0;

    ssize_t sent = sendto(sockfd, &pkt, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    //print_binario(&pkt, SLOW_HEADER_SIZE);
    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);

    return sent == SLOW_HEADER_SIZE;
}

bool SlowClient::receive_setup() {
    SlowPacket response{};
    ssize_t received = recvfrom(sockfd, &response, sizeof(response), 0, nullptr, nullptr);
    if (received < SLOW_HEADER_SIZE)
        return false;

    // Copiar nova sessão
    std::memcpy(session_id, response.sid, UUID_SIZE);
    session_ttl = response.sttl_flags;
    seqnum = response.seqnum;
    acknum = response.seqnum;
    window_size = response.window;

    //print_binario(&response, SLOW_HEADER_SIZE);
    print_packet_info(response, received, 1);

    return true;
}

bool SlowClient::send_data(const uint8_t* data, size_t length) {
    if (length > MAX_DATA_SIZE) return false;

    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK);
    pkt.seqnum = ++seqnum;
    pkt.acknum = acknum;
    pkt.window = window_size;
    pkt.fid = 0;
    pkt.fo = 0;
    std::memcpy(pkt.data, data, length);

    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum     = htonl(pkt.seqnum);
    pkt_net.acknum     = htonl(pkt.acknum);
    pkt_net.window     = htons(pkt.window);

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE + length, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    // print_binario(&pkt, SLOW_HEADER_SIZE + length);
    print_packet_info(pkt, SLOW_HEADER_SIZE + length, 0);

    return sent >= (ssize_t)(SLOW_HEADER_SIZE + length);
}


bool SlowClient::send_ack() {
    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK);

    pkt.acknum = seqnum;
    pkt.seqnum = seqnum;
    pkt.window = window_size;
    pkt.fid = 0;
    pkt.fo = 0;

    ssize_t sent = sendto(sockfd, &pkt, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));
    
    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);
                        
    return sent >= (ssize_t)(SLOW_HEADER_SIZE);
}

bool SlowClient::send_disconnect() {
    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);
    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK | FLAG_CONNECT | FLAG_REVIVE); 

    pkt.acknum = 0;
    pkt.seqnum = ++seqnum;
    pkt.window = 0;
    pkt.fid = 0;
    pkt.fo = 0;

    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum     = htonl(pkt.seqnum);
    pkt_net.acknum     = htonl(pkt.acknum);
    pkt_net.window     = htons(pkt.window);

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    // print_binario(&pkt, SLOW_HEADER_SIZE);
    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);

    return sent == SLOW_HEADER_SIZE;
}

bool SlowClient::receive_response() {
    SlowPacket response{};
    ssize_t received = recvfrom(sockfd, &response, sizeof(response), 0, nullptr, nullptr);
    if (received < SLOW_HEADER_SIZE) {
        std::cerr << ">> Nenhuma resposta recebida (timeout ou erro)\n";
        return false;
    }

    SlowPacket printable = response;
    printable.seqnum = ntohl(response.seqnum);
    printable.acknum = ntohl(response.acknum);

    print_packet_info(printable, received, 1);

    session_ttl = response.sttl_flags & 0x07FFFFFF;
    acknum = response.acknum;
    window_size = response.window;

    return true;
}
