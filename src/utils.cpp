#include "utils.hpp"

uint32_t encode_sttl_flags(uint32_t sttl, uint8_t flags) {
    return ((sttl & 0x07FFFFFF) << 5) | (flags & 0x1F);
}

void gerar_nil(uint8_t* uuid) {
    std::memset(uuid, 0, UUID_SIZE);
}

void print_uuid(const uint8_t* uuid) {
    for (int i = 0; i < UUID_SIZE; ++i)
        printf("%02x", uuid[i]);
    printf("\n");
}

void print_flags(uint8_t flags) {
    std::cout << std::bitset<5>(flags) << " (";
    bool printed = false;

    if (flags & FLAG_CONNECT) { std::cout << "CONNECT"; printed = true; }
    if (flags & FLAG_REVIVE)  { if (printed) std::cout << ", "; std::cout << "REVIVE"; printed = true; }
    if (flags & FLAG_ACK)     { if (printed) std::cout << ", "; std::cout << "ACK"; printed = true; }
    if (flags & FLAG_ACCEPT)  { if (printed) std::cout << ", "; std::cout << "ACCEPT"; printed = true; }
    if (flags & FLAG_MB)      { if (printed) std::cout << ", "; std::cout << "MB"; printed = true; }
    std::cout << ")\n";
}

uint32_t reverse_bits_32(uint32_t n) {
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result <<= 1;
        result |= (n & 1);
        n >>= 1;
    }
    return result;
}