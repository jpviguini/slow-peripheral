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

        client.debug_print_pacotes_pendentes();

        std::cout << "\n>> Aguardando resposta do servidor...\n";
        if (!client.receive_setup())
            throw std::runtime_error("Sem resposta do servidor");

        client.debug_print_pacotes_pendentes();

        // Dados para envio
        std::cout << "\n>> Enviando dados de teste...\n";

        const char* msg = "oi";
        if (!client.send_data(reinterpret_cast<const uint8_t*>(msg), strlen(msg)))
            throw std::runtime_error("Falha ao enviar dados");

        client.debug_print_pacotes_pendentes();

        // Recebe ACK da mensagem de dados
        if (!client.receive_response())
            std::cerr << ">> Aviso: não houve resposta para os dados enviados.\n";

        client.debug_print_pacotes_pendentes();

        // Envia sinal de encerramento
        std::cout << "\n>> Enviando DISCONNECT...\n";
        if (!client.send_disconnect())
            throw std::runtime_error("Falha ao desconectar");

        client.debug_print_pacotes_pendentes();

        // Recebe ACK da desconexão
        if (!client.receive_response())
            std::cerr << ">> Aviso: não houve resposta ao disconnect.\n";

        client.debug_print_pacotes_pendentes();

        std::cout << "\n>> Sessão encerrada com sucesso.\n";

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
