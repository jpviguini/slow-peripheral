#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <string>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctime>
#include <bitset>
#include <iostream>

// Defines do pacote
#define UUID_SIZE 16
#define MAX_DATA_SIZE 1440
#define SLOW_HEADER_SIZE 32
#define SERVER_PORT 7033

// Flags
#define FLAG_CONNECT 0x10
#define FLAG_REVIVE  0x08
#define FLAG_ACK     0x04
#define FLAG_ACCEPT  0x02
#define FLAG_MB      0x01

// Utilitários
uint32_t encode_sttl_flags(uint32_t sttl, uint8_t flags);
void gerar_nil(uint8_t* uuid);
void print_flags(uint8_t flags);
void print_uuid(const uint8_t* uuid);

#endif // UTILS_HPP
