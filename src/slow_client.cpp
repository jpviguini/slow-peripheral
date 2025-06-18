#include "utils.cpp"
#include <fstream>

SlowClient::SlowClient(const std::string& server_ip) {
    srand(time(nullptr)); // Inicializa gerador de números aleatórios para eventual UUID

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Cria socket UDP
    if (sockfd < 0)
        throw std::runtime_error("Erro ao criar socket");

    struct timeval tv = {TIMEOUT_RECVFROM, 0}; // Timeout de 5 segundos para recvfrom
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
    gerar_nil(session_id);
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


bool SlowClient::process_received_packet(SlowPacket& packet_out, ssize_t& received_bytes, bool flag_print) {
    received_bytes = recvfrom(sockfd, &packet_out, sizeof(packet_out), 0, nullptr, nullptr);

    if (received_bytes < SLOW_HEADER_SIZE) {
        if (janela_tem_pacotes_pendentes()) {
            std::cerr << "\n>> Nenhuma resposta recebida (timeout ou erro)\n";
        }
        return false;
    }

    if (!flag_print) {
        // Converte para exibição e lógica
        SlowPacket printable = packet_out;

        printable.seqnum = ntohl(packet_out.seqnum);
        printable.acknum = ntohl(packet_out.acknum);
        print_packet_info(printable, received_bytes, 1);
    } else {
        print_packet_info(packet_out, received_bytes, 1);
    }

    // Atualiza estado do cliente
    session_ttl = (packet_out.sttl_flags >> 5) & 0x07FFFFFF;

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
    if (!process_received_packet(response, received, 1))
        return false;

    // Inicializa nova sessão a partir da resposta
    std::memcpy(session_id, response.sid, UUID_SIZE);
    seqnum = response.seqnum;
    acknum = response.seqnum;

    janela_envio.confirmar_recebimento(acknum);

    return true;
}

bool SlowClient::receive_revive() {
    SlowPacket response{};
    ssize_t received;    // problema no received. tem que mandar o anterior?
 
    if (!process_received_packet(response, received, 1))
        return false;

    // Testa se o servidor devolve a flag de aceite
    if (!(response.sttl_flags & FLAG_ACCEPT)) {
        std::cerr << ">> Revive rejeitado pelo servidor\n";
        return false;  // servidor rejeitou o revive
    }

    // Restaura os dados da sessão
    std::memcpy(session_id, response.sid, UUID_SIZE);
    seqnum = response.seqnum;
    acknum = response.seqnum;

    janela_envio.confirmar_recebimento(acknum);

    return true;
}

bool SlowClient::receive_response() {
    SlowPacket response{};
    ssize_t received;
    return process_received_packet(response, received, 0);
}

bool SlowClient::send_data(const uint8_t* data, size_t length) {
    if (length > MAX_DATA_SIZE){
        std::cout << "Mensagem excede o limite de bits, fragmentando...\n\n"; 
        return send_fragmented_data(data, length); 
    }

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

    std::cout << "TTL (27 bits): " << std::bitset<32>(pkt.sttl_flags) << std::endl;
    
    print_packet_info(pkt, SLOW_HEADER_SIZE + length, 0);

    return sent >= (ssize_t)(SLOW_HEADER_SIZE + length);
}

void SlowClient::salvar_sessao_em_arquivo() const {
    std::ofstream arquivo("session.dat", std::ios::binary);
    if (!arquivo) return;

    arquivo.write(reinterpret_cast<const char*>(session_id), UUID_SIZE);

    // pega o tempo absoluto
    auto agora = std::chrono::system_clock::now().time_since_epoch();
    auto segundos = std::chrono::duration_cast<std::chrono::seconds>(agora).count();

    arquivo.write(reinterpret_cast<const char*>(&segundos), sizeof(int64_t));

    arquivo.write(reinterpret_cast<const char*>(&session_ttl), sizeof(uint32_t));
}


bool SlowClient::carregar_sessao_do_arquivo() {
    std::ifstream arquivo("session.dat", std::ios::binary);
    if (!arquivo) return false;

    arquivo.read(reinterpret_cast<char*>(session_id), UUID_SIZE);

    int64_t segundos_desde_epoca;
    arquivo.read(reinterpret_cast<char*>(&segundos_desde_epoca), sizeof(int64_t));

    arquivo.read(reinterpret_cast<char*>(&session_ttl), sizeof(uint32_t));

    // Pega tempo atual nos dois relógios
    auto agora_system = std::chrono::system_clock::now();
    auto agora_steady = std::chrono::steady_clock::now();

    // Reconstroi o tempo salvo no arquivo como time_point do system_clock
    auto tempo_salvo_system = std::chrono::system_clock::time_point(std::chrono::seconds(segundos_desde_epoca));

    // Calcula o tempo decorrido desde que salvou a sessão 
    auto diff = agora_system - tempo_salvo_system;

    // Ajusta session_start_time subtraindo o tempo decorrido
    session_start_time = agora_steady - std::chrono::duration_cast<std::chrono::steady_clock::duration>(diff);

    return true;
}

bool SlowClient::has_valid_session() const {
    // Verifica se o session_id está definido 
    if (is_nil_uuid(session_id)) {
        return false;
    }

    // Verifica se a sessão ainda está no tempo válido
    auto agora = std::chrono::steady_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::seconds>(agora - session_start_time);

    return duracao.count() < session_ttl;
}

bool SlowClient::is_nil_uuid(const uint8_t* uuid) const {
    for (int i = 0; i < UUID_SIZE; ++i) {
        if (uuid[i] != 0) return false;
    }
    return true;
}

bool SlowClient::send_revive() {
    SlowPacket pkt{};
    std::memcpy(pkt.sid, session_id, UUID_SIZE);

    pkt.sttl_flags = encode_sttl_flags(session_ttl, FLAG_ACK | FLAG_REVIVE);  // Revive ligado
    pkt.seqnum = ++seqnum;
    pkt.acknum = seqnum - 1;
    pkt.window = janela_envio.calcular_tamanho_disponivel();

    // Conversão para formato de rede
    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum = htonl(pkt.seqnum);
    pkt_net.acknum = htonl(pkt.acknum);
    pkt_net.window = htons(pkt.window);

    // janela_envio.registrar_envio(pkt.seqnum, pkt);

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    print_packet_info(pkt, SLOW_HEADER_SIZE, 0);
    
    return sent == (ssize_t)SLOW_HEADER_SIZE;
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

bool SlowClient::janela_tem_pacotes_pendentes() const {
    return janela_envio.calcular_tamanho_disponivel() < 1024;
}

bool SlowClient::reenviar_pacote(const SlowPacket& pkt, int reenviado) {
    SlowPacket pkt_net = pkt;
    pkt_net.sttl_flags = htonl(pkt.sttl_flags);
    pkt_net.seqnum = htonl(pkt.seqnum);
    pkt_net.acknum = htonl(pkt.acknum);
    pkt_net.window = htons(pkt.window);

    size_t data_length = strlen(reinterpret_cast<const char*>(pkt_net.data));

    ssize_t sent = sendto(sockfd, &pkt_net, SLOW_HEADER_SIZE + data_length, 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));

    print_packet_info(pkt, SLOW_HEADER_SIZE + data_length, 2, reenviado);
    return sent == (SLOW_HEADER_SIZE + 2);
}
