#include "slow_client.hpp"
#include "slow_threads.hpp" // Gerenciador de threads
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

void loop_de_mensagens(SlowThreadManager& threads) {
    std::string entrada;
    while (true) {
        std::cout << "\n> Digite sua mensagem (ou 'sair'): ";
        std::getline(std::cin, entrada);
        if (entrada == "sair") break;

        std::vector<uint8_t> dados(entrada.begin(), entrada.end());
        threads.enfileirar_mensagem(dados);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void desconectar(SlowClient& client) {
    std::cout << "\n>> Enviando DISCONNECT...\n";
    if (!client.send_disconnect())
        throw std::runtime_error("Falha ao desconectar");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    while (client.janela_tem_pacotes_pendentes()) {
        // Esperando pacotes pendentes esvaziarem
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void tentar_revive(SlowClient& client) {
    std::cout << "\n>> Tentando REVIVE após DISCONNECT...\n";
    if (client.send_revive()) {
        std::cerr << ">> Sucesso ao enviar pacote REVIVE.\n";
    } else {
        std::cerr << ">> Falha ao enviar pacote REVIVE.\n";
    }
}

int main() {
    try {
        SlowClient client("142.93.184.175");

        std::cout << ">> Conectando com CONNECT...\n";
        if (!client.send_connect()) throw std::runtime_error("CONNECT falhou");
        if (!client.receive_setup()) throw std::runtime_error("SETUP não recebido");

        SlowThreadManager threads(client);
        threads.iniciar();

        // Tirei esse ack, uma vez que já evio o data com um ACK, ou seja, 
        // ele já realiza a função de ACK + Dados (e po ter fragment também).
        // client.send_ack();

        std::cout << ">> SETUP realizado com sucesso!.\n";

        std::this_thread::sleep_for(std::chrono::seconds(3));

        loop_de_mensagens(threads);
        client.debug_print_pacotes_pendentes();

        desconectar(client);

        threads.parar();

        std::cout << "\n>> Parando as Threads.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "\n>> Ligando as Threads (Para testar Revive).\n";

        threads.iniciar();

        tentar_revive(client);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        loop_de_mensagens(threads);
        desconectar(client);

        threads.parar();
        std::cout << "\n>> Desligando client.\n";

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
