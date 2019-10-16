#ifndef WS_LIBRARY_CRC_H
#define WS_LIBRARY_CRC_H

#include <cstdint>
#include <type_traits>

namespace _crc_{

extern uint8_t crc8(const void* data, uint32_t len);

extern void add_crc8(void* data, uint32_t len);

extern bool verify_crc8(const void* data, uint32_t len);

extern uint8_t crc8_continue(
    uint8_t prev_crc, const void* data, uint32_t len);

extern void free_table();

}

#endif  

