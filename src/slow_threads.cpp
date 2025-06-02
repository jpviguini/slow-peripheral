#include "slow_threads.hpp"
#include "slow_client.hpp" 

SlowThreadManager::SlowThreadManager(SlowClient& client_ref)
    : client(client_ref) {}

SlowThreadManager::~SlowThreadManager() {
    parar();
}

void SlowThreadManager::iniciar() {
    executando = true;
    t_envio = std::thread(&SlowThreadManager::thread_envio, this);
    t_recebimento = std::thread(&SlowThreadManager::thread_recebimento, this);
}

void SlowThreadManager::parar() {
    executando = false;
    cond_var.notify_all();

    if (t_envio.joinable()) t_envio.join();
    if (t_recebimento.joinable()) t_recebimento.join();
}

void SlowThreadManager::enfileirar_mensagem(const std::vector<uint8_t>& dados) {
    {
        std::lock_guard<std::mutex> lock(mutex_fila);
        fila_envio.push(dados);
    }
    cond_var.notify_one();
}

void SlowThreadManager::thread_envio() {
    while (executando) {
        std::vector<uint8_t> dados;

        {
            std::unique_lock<std::mutex> lock(mutex_fila);
            cond_var.wait(lock, [&]() {
                return !fila_envio.empty() || !executando;
            });

            if (!executando) break;
            dados = fila_envio.front();
            fila_envio.pop();
        }

        if (!client.send_data(dados.data(), dados.size())) {
            std::cerr << "[ERRO] Falha no envio de dados pela thread.\n";
        }
    }
}

void SlowThreadManager::thread_recebimento() {
    while (executando) {
        SlowPacket pkt{};
        ssize_t bytes = 0;

        if (client.process_received_packet(pkt, bytes)) {
            // ACKs já processados internamente
        } else {
            // Só exibe aviso se há pacotes pendentes aguardando ACK
            if (client.janela_tem_pacotes_pendentes()) {
                std::cerr << ">> Nenhuma resposta recebida (timeout ou erro)\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
    }
}
