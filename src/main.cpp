#include "slow_client.hpp"
#include "slow_threads.hpp" // Gerenciador de threads
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <chrono>

int main() {
    try {
        SlowClient client("142.93.184.175");
        bool revive_ok = false;

        if (client.carregar_sessao_do_arquivo() && client.has_valid_session()) {
            std::cout << ">> Tentando revive com sessão anterior...\n";
            revive_ok = client.send_revive();
            std::cout << "revive enviado? (1=sim, 0=nao) " << revive_ok << "\n";

            if (!client.receive_revive()) throw std::runtime_error("REVIVE não recebido");
        }

        if (!revive_ok) {
            std::cout << ">> Sessão inválida ou revive falhou, conectando normalmente...\n";
            if (!client.send_connect()) throw std::runtime_error("CONNECT falhou");
            if (!client.receive_setup()) throw std::runtime_error("SETUP não recebido");


            client.salvar_sessao_em_arquivo(); // Salva nova sessão para uso futuro
        }

        client.debug_print_pacotes_pendentes();

        // Inicia as threads de envio e recebimento
        SlowThreadManager threads(client);
        threads.iniciar();

        // Enfileira mensagens para envio
        std::string entrada;
        while (true) {
            std::cout << "\n> Digite sua mensagem (ou 'sair'): ";
            std::getline(std::cin, entrada);

            if (entrada == "sair") break;

            std::vector<uint8_t> dados(entrada.begin(), entrada.end());
            threads.enfileirar_mensagem(dados);

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        client.debug_print_pacotes_pendentes();

        std::cout << "\n>> Enviando DISCONNECT...\n";
        if (!client.send_disconnect())
            throw std::runtime_error("Falha ao desconectar");

        threads.parar();
        std::cout << "\n>> Sessão encerrada com sucesso.\n";

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
