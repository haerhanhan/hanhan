#include "crc.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <mutex>

std::vector<uint16_t> crc16_table;
std::vector<uint32_t> crc32_table;
std::vector<uint8_t> crc8_table;
std::mutex mtx;

namespace _crc_{


void init_crc8_table()
{  // {{{
  if (!crc8_table.empty())
  {
    return;
  }
  crc8_table.resize(256);
  uint16_t i,j;
  uint8_t data;
  
  for (i = 0; i < 256; i++)
  {
    data = i;
    for (j = 0; j < 8; j++)
    {
      if ((data & 0x80) > 0)
        data = (data << 1) ^ 0x07;
      else
        data <<= 1;
    }
    crc8_table[i] = data & 0xFF;
  }
}  // }}}

uint8_t crc8_impl(const void* data, uint32_t len, uint8_t prev_crc = 0x00)
{  // {{{

  std::lock_guard<std::mutex>lck(mtx);
  if (crc8_table.empty())
  {
    init_crc8_table();
  }
  uint8_t crc = prev_crc;
  const uint8_t* p = static_cast<const uint8_t*>(data);
  for (uint32_t i = 0; i < len; ++i)
  {
    crc = crc8_table[crc ^ p[i]];
  }
  
  return crc;
}  // }}}

uint8_t crc8(const void* data, uint32_t len)
{  // {{{
  return crc8_impl(data, len);
}  // }}}

void add_crc8(void* data, uint32_t len)
{  // {{{
  auto crc = crc8(data, len);
  auto p = static_cast<uint8_t*>(data);
  p[len] = crc;
  //std::cout<<std::hex<<(unsigned int )(unsigned char)crc<<std::endl;
}  // }}}

bool verify_crc8(const void* data, uint32_t len)
{  // {{{
  return crc8(data, len) == 0;
}  // }}}

uint8_t crc8_continue(uint8_t prev_crc, const void* data, uint32_t len)
{  // {{{
  return crc8_impl(data, len, prev_crc);
}  // }}}

void free_table()
{
  crc8_table.clear();
}

}

// vim: fdm=marker

