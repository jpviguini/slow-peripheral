#pragma once
#include <cstdint>

#define UUID_SIZE 16
#define MAX_DATA_SIZE 1440
#define SLOW_HEADER_SIZE 32

struct SlowPacket {
    uint8_t sid[UUID_SIZE];     // Session ID
    uint32_t sttl_flags;        // STTL e flags
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t window;
    uint8_t fid;
    uint8_t fo;
    uint8_t data[MAX_DATA_SIZE]; // Dados (payload)
};
