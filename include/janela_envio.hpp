#pragma once
#include <map>
#include <mutex>
#include <vector>
#include <iostream>
#include "slow_packet.hpp"
#include <chrono>

using Clock = std::chrono::steady_clock;
using TempoEnvio = Clock::time_point;
using Reenviado = int;

/**
 * Classe responsável por gerenciar os pacotes enviados
 * que ainda não foram confirmados (sem ACK).
 */
class JanelaEnvio {
public:
    // Adiciona um novo pacote enviado para o buffer
    void registrar_envio(uint32_t seqnum, const SlowPacket& pacote);

    // Remove o pacote correspondente ao número de sequência confirmado por ACK
    void confirmar_recebimento(uint32_t acknum);

    // Retorna quantos slots ainda estão disponíveis na janela (para campo window)
    uint16_t calcular_tamanho_disponivel() const;

    // Retorna todos os pacotes pendentes de ACK
    std::vector<SlowPacket> listar_pendentes() const;

    // Imprime todos os pacotes do buffer
    void imprimir_pacotes_pendentes() const;

    void remover_pacote(uint32_t seqnum);

    std::vector<std::tuple<uint32_t, SlowPacket, Reenviado>> verificar_timeouts(std::chrono::milliseconds limite);

private:
    mutable std::mutex mutex_buffer;
    std::map<uint32_t, std::pair<SlowPacket, std::pair<TempoEnvio, Reenviado>>> pacotes_pendentes;

    // Tamanho máximo da janela de envio (buffer local)
    static constexpr size_t TAMANHO_JANELA = 1024;
};
