#include "slow_threads.hpp"
#include "slow_client.hpp" 

SlowThreadManager::SlowThreadManager(SlowClient& client_ref)
/* Armazena a referência ao cliente utilizado para envio/recebimento */
    : client(client_ref) {}

SlowThreadManager::~SlowThreadManager() {
/* Para e aguarda finalização das threads antes de destruir */
    parar();
}

void SlowThreadManager::iniciar() {
/* Aciona as threads de envio e recebimento */
    executando = true;
    t_envio = std::thread(&SlowThreadManager::thread_envio, this);
    t_recebimento = std::thread(&SlowThreadManager::thread_recebimento, this);
}

void SlowThreadManager::parar() {
/* Sinaliza parada, notifica e faz join em ambas as threads */
    executando = false;
    cond_var.notify_all();

    if (t_envio.joinable()) t_envio.join();
    if (t_recebimento.joinable()) t_recebimento.join();
}

void SlowThreadManager::enfileirar_mensagem(const std::vector<uint8_t>& dados) {
/* Adiciona dados à fila de envio e notifica a thread de envio */
    {
        std::lock_guard<std::mutex> lock(mutex_fila);
        fila_envio.push(dados);
    }
    cond_var.notify_one();
}

void SlowThreadManager::thread_envio() {
/* Loop de envio: aguarda fila, envia dados e reenvia pacotes com timeout */
    using namespace std::chrono_literals;

    while (executando) {
        std::vector<uint8_t> dados;

        {
            std::unique_lock<std::mutex> lock(mutex_fila);
            cond_var.wait_for(lock, 100ms, [&]() {
                return !fila_envio.empty() || !executando;
            });

            if (!executando) break;

            if (!fila_envio.empty()) {
                dados = fila_envio.front();
                fila_envio.pop();
            }
        }

        // Se havia dados, envia
        if (!dados.empty()) {
            if (!client.send_data(dados.data(), dados.size())) {
                std::cerr << "[ERRO] Falha no envio de dados pela thread.\n";
            }
        }

        // Verifica pacotes que precisam de reenvio
        auto reenviaveis = client.get_janela_envio().verificar_timeouts(std::chrono::milliseconds(TEMPO_REENVIO));
        for (const auto& [seq, pkt, reenviado] : reenviaveis) {
            client.reenviar_pacote(pkt, reenviado);
        }

        std::this_thread::sleep_for(10ms);
    }
}

void SlowThreadManager::thread_recebimento() {
/* Loop de recebimento: processa pacotes recebidos e exibe debug */
    while (executando) {
        SlowPacket pkt{};
        ssize_t bytes = 0;

        if (client.process_received_packet(pkt, bytes, 0)) {
            client.debug_print_pacotes_pendentes();
        } 

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
    }
}