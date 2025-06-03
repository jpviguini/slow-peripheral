#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

#define TEMPO_REENVIO 3000

class SlowClient;

class SlowThreadManager {
public:
    explicit SlowThreadManager(SlowClient& client_ref);
    ~SlowThreadManager();

    void iniciar();
    void parar();
    void enfileirar_mensagem(const std::vector<uint8_t>& dados);

private:
    void thread_envio();
    void thread_recebimento();

    SlowClient& client;
    std::thread t_envio, t_recebimento;

    std::queue<std::vector<uint8_t>> fila_envio;
    std::mutex mutex_fila;
    std::condition_variable cond_var;

    std::atomic<bool> executando{false};
};
