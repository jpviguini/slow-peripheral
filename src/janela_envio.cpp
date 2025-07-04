#include "janela_envio.hpp"

void JanelaEnvio::registrar_envio(uint32_t seqnum, const SlowPacket& pacote) 
/* Adiciona um novo pacote à janela de pendentes se houver espaço disponível */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);

    if (pacotes_pendentes.size() < TAMANHO_JANELA) {
        pacotes_pendentes[seqnum] = {pacote, {Clock::now(), 0}};
    }
}

void JanelaEnvio::confirmar_recebimento(uint32_t acknum) 
/* Remove o pacote correspondente ao ACK recebido da janela de pendentes */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);

    // Remove o pacote correspondente ao acknum
    auto it = pacotes_pendentes.find(acknum);
    if (it != pacotes_pendentes.end()) {
        pacotes_pendentes.erase(it);
    }
}

uint16_t JanelaEnvio::calcular_tamanho_disponivel() const 
/* Retorna a quantidade de slots livres na janela de envio */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);

    // Retorna o espaço restante na janela (campo window)
    return static_cast<uint16_t>(TAMANHO_JANELA - pacotes_pendentes.size());
}

std::vector<SlowPacket> JanelaEnvio::listar_pendentes() const 
/* Gera e retorna um vetor com todos os pacotes ainda não confirmados */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);

    std::vector<SlowPacket> lista;
    for (const auto& par : pacotes_pendentes)
        lista.push_back(par.second.first);

    return lista;
}

void JanelaEnvio::imprimir_pacotes_pendentes() const 
/* Exibe no console detalhes de cada pacote pendente na janela */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);

    std::cout << "\n--- Pacotes Pendentes na Janela ---\n";

    if (pacotes_pendentes.empty()) {
        std::cout << "Nenhum pacote pendente.\n";
        return;
    }

    for (const auto& [seq, par] : pacotes_pendentes) {
        const auto& pkt = par.first;  // par é um std::pair<SlowPacket, time_point>
        std::cout << "SEQNUM: " << pkt.seqnum
                << " | ACKNUM: " << pkt.acknum
                << " | FID: " << static_cast<int>(pkt.fid)
                << " | FO: " << static_cast<int>(pkt.fo)
                << " | WINDOW: " << pkt.window
                << " | Reenviado: " << par.second.second
                << "\n";
    }


    std::cout << "------------------------------------\n";
}

std::vector<std::tuple<uint32_t, SlowPacket, Reenviado>> JanelaEnvio::verificar_timeouts(std::chrono::milliseconds limite) 
/* Verifica quais pacotes excederam o tempo limite e prepara para reenvio */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);
    std::vector<std::tuple<uint32_t, SlowPacket, Reenviado>> para_reenvio;
    auto agora = Clock::now();

    for (auto& [seq, par] : pacotes_pendentes) {
        auto& [pkt, tempo_info] = par;
        auto& [ultimo_envio, reenviado] = tempo_info;

        if (agora - ultimo_envio > limite) {
            reenviado++;  // Atualiza contador de reenvios
            ultimo_envio = agora;  // Atualiza tempo para evitar reenvio contínuo
            para_reenvio.emplace_back(seq, pkt, reenviado);
        }
    }

    return para_reenvio;
}

void JanelaEnvio::remover_pacote(uint32_t seqnum)
/* Remove explicitamente um pacote da janela de envio */
{
    std::lock_guard<std::mutex> trava(mutex_buffer);
    pacotes_pendentes.erase(seqnum);
}


