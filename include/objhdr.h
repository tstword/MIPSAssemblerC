#ifndef OBJHDR_H
#define OBJHDR_H

#include <stdint.h>
#include "symtable.h"

struct mips_obj_fhdr {
    uint8_t m_padding[1];
    uint8_t m_magic[4];
    uint8_t m_endianness;
    uint8_t m_version;
    uint8_t m_shnum;
};

struct mips_obj_shdr {
    uint8_t   sh_padding[3];
    segment_t sh_segment;
    offset_t  sh_offset;
    offset_t  sh_size;
};

#endif