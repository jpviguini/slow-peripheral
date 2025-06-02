#include "utils.cpp"

SlowClient::SlowClient(const std::string& server_ip) {
    srand(time(nullptr)); // Inicializa gerador de números aleatórios para eventual UUID

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Cria socket UDP
    if (sockfd < 0)
        throw std::runtime_error("Erro ao criar socket");

    struct timeval tv = {5, 0}; // Timeout de 5 segundos para recvfrom
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Configuração do endereço do servidor
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    gerar_nil(session_id); // Inicia sessão com UUID nulo
    seqnum = 0;
    acknum = 0;
    window_size = janela_envio.calcular_tamanho_disponivel(); // Tamanho da janela inicial
}


SlowClient::~SlowClient() {
    close(sockfd); // Fecha o socket ao destruir o cliente
}


bool SlowClient::send_connect() {
    SlowPacket pkt{};
    std::memset(&pkt, 0, sizeof(pkt));
    std::memcpy(pkt.sid, session_id, UUID_SIZE); // Envia UUID nulo

    pkt.sttl_flags = encode_sttl_flags(0, FLAG_CONNECT); // Sinaliza tentativa de conexão
    pkt.seqnum = 0;
    pkt.acknum = 0;
    pkt.window = window_size;

    janela_envio.registrar_envio(pkt.seqnum, pkt);

    ssize_t sent = sendto(sockfd, &pkt, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);
    return sent == SLOW_HEADER_SIZE;
}


bool SlowClient::process_received_packet(SlowPacket& packet_out, ssize_t& received_bytes) {
    received_bytes = recvfrom(sockfd, &packet_out, sizeof(packet_out), 0, nullptr, nullptr);
    if (received_bytes < SLOW_HEADER_SIZE) {
        std::cerr << ">> Nenhuma resposta recebida (timeout ou erro)\n";
        return false;
    }

    // Converte para exibição e lógica
    SlowPacket printable = packet_out;
    printable.seqnum = ntohl(packet_out.seqnum);
    printable.acknum = ntohl(packet_out.acknum);

    print_packet_info(printable, received_bytes, 1);

    // Atualiza estado do cliente
    session_ttl = packet_out.sttl_flags & 0x07FFFFFF;
    acknum = packet_out.acknum;
    window_size = packet_out.window;

    // Confirma recebimento do pacote correspondente ao ACK
    uint32_t ack_confirmado = ntohl(packet_out.acknum);
    janela_envio.confirmar_recebimento(ack_confirmado);

    return true;
}


bool SlowClient::receive_setup() {
    SlowPacket response{};
    ssize_t received;
    if (!process_received_packet(response, received))
        return false;

    // Inicializa nova sessão a partir da resposta
    std::memcpy(session_id, response.sid, UUID_SIZE);
    seqnum = response.seqnum;
    acknum = response.seqnum;

    janela_envio.confirmar_recebimento(acknum);

    return true;
}

bool SlowClient::receive_response() {
    SlowPacket response{};
    ssize_t received;
    return process_received_packet(response, received);
}

bool SlowClient::send_data(const uint8_t* data, size_t length) {
    if (length > MAX_DATA_SIZE) return false;

    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK);
    pkt.seqnum = ++seqnum;
    pkt.acknum = acknum;
    pkt.window = janela_envio.calcular_tamanho_disponivel();

    std::memcpy(pkt.data, data, length);

    // Conversão para rede
    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum = htonl(pkt.seqnum);
    pkt_net.acknum = htonl(pkt.acknum);
    pkt_net.window = htons(pkt.window);

    janela_envio.registrar_envio(pkt.seqnum, pkt);

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE + length, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    print_packet_info(pkt, SLOW_HEADER_SIZE + length, 0);

    return sent >= (ssize_t)(SLOW_HEADER_SIZE + length);
}


bool SlowClient::send_ack() {
    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK);
    pkt.acknum = seqnum;
    pkt.seqnum = seqnum;
    pkt.window = janela_envio.calcular_tamanho_disponivel();

    janela_envio.registrar_envio(pkt.seqnum, pkt);

    ssize_t sent = sendto(sockfd, &pkt, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));
    
    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);
    return sent >= (ssize_t)(SLOW_HEADER_SIZE);
}


bool SlowClient::send_disconnect() {
    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    // Envia pacote com flags de desconexão
    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK | FLAG_CONNECT | FLAG_REVIVE);
    pkt.seqnum = ++seqnum;
    pkt.acknum = 0;
    pkt.window = 0;

    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum = htonl(pkt.seqnum);
    pkt_net.acknum = htonl(pkt.acknum);
    pkt_net.window = htons(pkt.window);

    janela_envio.registrar_envio(pkt.seqnum, pkt);

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);

    return sent == SLOW_HEADER_SIZE;
}

void SlowClient::debug_print_pacotes_pendentes() const {
    janela_envio.imprimir_pacotes_pendentes();
}

bool SlowClient::send_fragmented_data(const uint8_t* data, size_t length) {
    size_t offset = 0;
    uint16_t fragment_id = ++fid_counter; // Novo FID para essa mensagem

    uint32_t fragment_offset = 0; // fo começa em 0

    while (offset < length) {
        size_t chunk_size = std::min(length - offset, (size_t)MAX_DATA_SIZE);

        SlowPacket pkt{};
        std::memcpy(pkt.sid, session_id, UUID_SIZE);

        // Ativa a flag MB apenas se ainda restarem fragmentos após este
        uint8_t flags = FLAG_ACK;
        if (offset + chunk_size < length) {
            flags |= FLAG_MB; // MB ligada nos fragmentos intermediários
        }
        pkt.sttl_flags = encode_sttl_flags(session_ttl, flags);

        pkt.seqnum = ++seqnum;
        pkt.acknum = acknum;
        pkt.window = janela_envio.calcular_tamanho_disponivel();
        pkt.fid = fragment_id;
        pkt.fo = fragment_offset++;
        std::memcpy(pkt.data, data + offset, chunk_size);

        SlowPacket pkt_net = pkt;
        pkt_net.sttl_flags = htonl(pkt.sttl_flags);
        pkt_net.seqnum = htonl(pkt.seqnum);
        pkt_net.acknum = htonl(pkt.acknum);
        pkt_net.window = htons(pkt.window);

        ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE + chunk_size, 0,
                              (sockaddr*)&server_addr, sizeof(server_addr));

        print_packet_info(pkt, SLOW_HEADER_SIZE + chunk_size, 0);

        if (sent < (ssize_t)(SLOW_HEADER_SIZE + chunk_size)) {
            std::cerr << "Erro ao enviar fragmento na posição " << offset << "\n";
            return false;
        }

        offset += chunk_size;
        janela_envio.registrar_envio(pkt.seqnum, pkt);
    }

    return true;
}