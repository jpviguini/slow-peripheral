#include "slow_client.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

int main() {
    try {
        // Inicializa cliente com IP do servidor
        SlowClient client("142.93.184.175");

        std::cout << ">> Enviando pacote CONNECT...\n\n";
        if (!client.send_connect())
            throw std::runtime_error("Falha ao enviar CONNECT");

        std::cout << ">> Aguardando resposta do servidor...\n";
        if (!client.receive_setup())
            throw std::runtime_error("Sem resposta do servidor");

        // Dados para envio
        std::cout << ">> Enviando dados de teste...\n";

        // if (!client.send_ack())
        //     throw std::runtime_error("Falha ao enviar ACK");

        const char* msg = "oi";
        if (!client.send_data(reinterpret_cast<const uint8_t*>(msg), strlen(msg)))
            throw std::runtime_error("Falha ao enviar dados");

        // Recebe ACK da mensagem de dados
        if (!client.receive_response())
            std::cerr << ">> Aviso: não houve resposta para os dados enviados.\n";

        // Envia sinal de encerramento
        std::cout << ">> Enviando DISCONNECT...\n";
        if (!client.send_disconnect())
            throw std::runtime_error("Falha ao desconectar");

        // Recebe ACK da desconexão
        if (!client.receive_response())
            std::cerr << ">> Aviso: não houve resposta ao disconnect.\n";

        std::cout << ">> Sessão encerrada com sucesso.\n";

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
