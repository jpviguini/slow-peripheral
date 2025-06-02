#include "slow_client.hpp"
#include "slow_threads.hpp" // Gerenciador de threads
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <chrono>

int main() {
    try {
        // Inicializa cliente com IP do servidor
        SlowClient client("142.93.184.175");

        std::cout << ">> Enviando pacote CONNECT...\n";
        if (!client.send_connect())
            throw std::runtime_error("Falha ao enviar CONNECT");

        client.debug_print_pacotes_pendentes();

        std::cout << "\n>> Aguardando resposta de Setup do servidor...\n";
        if (!client.receive_setup())
            throw std::runtime_error("Sem resposta do servidor");

        client.debug_print_pacotes_pendentes();

        std::cout << "\n";

        // Inicia as threads de envio e recebimento
        SlowThreadManager threads(client);
        threads.iniciar();

        // Enfileira mensagens para envio
        std::string entrada;
        while (true) {
            std::cout << "> Digite sua mensagem (ou 'sair'): ";
            std::getline(std::cin, entrada);

            if (entrada == "sair") break;

            std::vector<uint8_t> dados(entrada.begin(), entrada.end());
            threads.enfileirar_mensagem(dados);

            std::this_thread::sleep_for(std::chrono::milliseconds(300));  // Aguarda 300 ms
        }

        // Espera a comunicação ocorrer
        std::this_thread::sleep_for(std::chrono::seconds(1));

        client.debug_print_pacotes_pendentes();

        // Envia sinal de encerramento
        std::cout << "\n>> Enviando DISCONNECT...\n";
        if (!client.send_disconnect())
            throw std::runtime_error("Falha ao desconectar");

        client.debug_print_pacotes_pendentes();

        // Finaliza as threads com segurança
        threads.parar();

        client.debug_print_pacotes_pendentes();

        std::cout << "\n>> Sessão encerrada com sucesso.\n";

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
