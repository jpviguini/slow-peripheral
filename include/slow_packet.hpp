#pragma once
#include <cstdint>

#define UUID_SIZE 16
#define MAX_DATA_SIZE 1440
#define SLOW_HEADER_SIZE 32

struct SlowPacket {
    uint8_t sid[UUID_SIZE];     // Identificador único da sessão (Session ID), usado para manter o contexto da comunicação.

    uint32_t sttl_flags;        // Campo combinado que armazena:
                                // - STTL (Session Time To Live): tempo de validade da sessão.
                                // - Flags de controle (por exemplo: CONNECT, ACK, DISCONNECT...).

    uint32_t seqnum;            // Número de sequência (Sequence Number): identifica a ordem dos pacotes enviados.

    uint32_t acknum;            // Número de reconhecimento (Acknowledgment Number): confirma o recebimento de pacotes anteriores.

    uint16_t window;            // Tamanho da janela (Window Size): quantidade de pacotes que podem ser enviados sem ACK (controle de fluxo).

    uint8_t fid;                // Fragment ID: identifica fragmentos de uma mesma mensagem (em caso de fragmentação).

    uint8_t fo;                 // Fragment Offset: deslocamento do fragmento no payload total.

    uint8_t data[MAX_DATA_SIZE]; // Dados (Payload): conteúdo da mensagem a ser transmitida, limitado a MAX_DATA_SIZE bytes.
};

