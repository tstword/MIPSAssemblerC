#ifndef MIPSFHDR_H
#define MIPSFHDR_H

#include <stdint.h>

struct MIPS_file_header {
    uint8_t m_magic[4];
    uint8_t m_endianness;
    uint8_t m_version;
    uint8_t m_shnum;
    uint8_t m_padding[1];
};

struct MIPS_sect_header {
    uint8_t sh_segment;
    uint8_t sh_padding[3];
    uint32_t sh_offset;
    uint32_t sh_size;
};

#endif